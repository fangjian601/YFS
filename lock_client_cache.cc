// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>

lock_releaser::lock_releaser(extent_client* _ec)
: ec(_ec){}

lock_releaser::~lock_releaser(){}

void lock_releaser::dorelease(lock_protocol::lockid_t lid)
{
	ec->flush(lid);
}

lock_info_client::lock_info_client(lock_protocol::lockid_t _id, status _stat)
: id(_id), stat(_stat){
	pthread_mutex_init(&lock_mutex, NULL);
	pthread_cond_init(&lock_cond, NULL);
	pthread_cond_init(&revoke_cond, NULL);
}

static void *
releasethread(void *x)
{
  lock_client_cache *cc = (lock_client_cache *) x;
  cc->releaser();
  return 0;
}

static void *
retrythread(void *x)
{
  lock_client_cache *cc = (lock_client_cache *) x;
  cc->retryer();
  return 0;
}

int lock_client_cache::last_port = 0;

lock_client_cache::lock_client_cache(std::string xdst, 
				     class lock_release_user *_lu)
  : lock_client(xdst), lu(_lu)
{
	rcl = new rsm_client(xdst);
	rlsrpc_init();
	rlsrpc_reg();
	rlsrpc_subscribe();
	last_request_id = 0;
	pthread_mutex_init(&locks_mutex, NULL);
	pthread_mutex_init(&releaser_mutex, NULL);
	pthread_cond_init(&releaser_cond, NULL);
	pthread_mutex_init(&retryer_mutex, NULL);
	pthread_cond_init(&retryer_cond, NULL);
	pthread_t revoker,retryer;
	int r = pthread_create(&revoker, NULL, &releasethread, (void *) this);
	assert (r == 0);
	r = pthread_create(&retryer, NULL, &retrythread, (void *) this);
	assert (r == 0);
}


void
lock_client_cache::rlsrpc_init(){
	  srand(time(NULL)^last_port);
	  rlock_port = ((rand()%32000) | (0x1 << 10));
	  hostname = "127.0.0.1";
	  std::ostringstream host;
	  host << hostname << ":" << rlock_port;
	  id = host.str();
	  last_port = rlock_port;
}

void
lock_client_cache::rlsrpc_reg(){
	  /* register RPC handlers with rlsrpc */
	  rpcs *rlsrpc = new rpcs(rlock_port);
	  rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry);
	  rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke);
}

void
lock_client_cache::rlsrpc_subscribe(){
	  int r;
	  int ret = rcl->call(lock_protocol::subscribe, hostname, rlock_port, r);
	  assert (ret == lock_protocol::OK);
}

lock_info_client*
lock_client_cache::get_lock(lock_protocol::lockid_t lid){
	pthread_mutex_lock(&locks_mutex);
	if(locks.find(lid) == locks.end()){
		lock_info_client* lock = new lock_info_client(lid);
		locks[lid] = lock;
		pthread_mutex_unlock(&locks_mutex);
		return lock;
	}
	else{
		pthread_mutex_unlock(&locks_mutex);
		return locks[lid];
	}
}

void
lock_client_cache::releaser()
{

	// This method should be a continuous loop, waiting to be notified of
	// freed locks that have been revoked by the server, so that it can
	// send a release RPC.
	while(true){
		pthread_mutex_lock(&releaser_mutex);
		pthread_cond_wait(&releaser_cond, &releaser_mutex);
		int size = revoke_list.size();
		for(int i=0; i<size; i++){
			lock_protocol::lockid_t lid = revoke_list.front();
			revoke_list.pop_front();

			lock_info_client* lock = get_lock(lid);
			pthread_mutex_lock(&lock->lock_mutex);
			if(lock->stat == lock_info_client::LOCKED){
				lock->stat = lock_info_client::RELEASING;
				pthread_cond_wait(&lock->revoke_cond, &lock->lock_mutex);
				assert(lock->stat == lock_info_client::RELEASING);
			}
			else {
				assert(lock->stat == lock_info_client::FREE);
			}
			int r;
			//lu->dorelease(lid);
			rcl->call(lock_protocol::release, id, ++last_request_id, lid, r);
			lock->stat = lock_info_client::NONE;
			pthread_cond_signal(&lock->lock_cond);
			pthread_mutex_unlock(&lock->lock_mutex);
		}
		pthread_mutex_unlock(&releaser_mutex);
	}

}

void
lock_client_cache::retryer()
{
	while(true){
		pthread_mutex_lock(&retryer_mutex);
		pthread_cond_wait(&retryer_cond, &retryer_mutex);
		int size = retry_list.size();
		for(int i=0; i<size; i++){
			lock_protocol::lockid_t lid = retry_list.front();
			retry_list.pop_front();

			lock_info_client* lock = get_lock(lid);
			pthread_mutex_lock(&lock->lock_mutex);
			int r;
			lock_protocol::status server_ret = rcl->call(lock_protocol::acquire, id, ++last_request_id, lid, r);
			assert(server_ret == lock_protocol::OK);
			assert(lock->stat == lock_info_client::ACQUIRING);
			lock->stat = lock_info_client::FREE;
			pthread_cond_signal(&lock->lock_cond);
			pthread_mutex_unlock(&lock->lock_mutex);
		}
		pthread_mutex_unlock(&retryer_mutex);

	}
}

rlock_protocol::status
lock_client_cache::retry(lock_protocol::lockid_t lid, int &)
{
	rlock_protocol::status ret = rlock_protocol::OK;
	pthread_mutex_lock(&retryer_mutex);
	retry_list.push_back(lid);
	pthread_cond_signal(&retryer_cond);
	pthread_mutex_unlock(&retryer_mutex);
	return ret;
}

rlock_protocol::status
lock_client_cache::revoke(lock_protocol::lockid_t lid, int &)
{
	rlock_protocol::status ret = rlock_protocol::OK;
	pthread_mutex_lock(&releaser_mutex);
	revoke_list.push_back(lid);
	pthread_cond_signal(&releaser_cond);
	pthread_mutex_unlock(&releaser_mutex);
	return ret;
}

lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid)
{
	lock_protocol::status ret;
	lock_info_client* lock = get_lock(lid);
	pthread_mutex_lock(&lock->lock_mutex);
	while(true){
		if(lock->stat == lock_info_client::NONE){
			int r;
			lock_protocol::status server_ret = rcl->call(lock_protocol::acquire, id, ++last_request_id, lid, r);
			if(server_ret == lock_protocol::OK){
				goto lock_free;
			}
			else if(server_ret == lock_protocol::RETRY){
				lock->stat = lock_info_client::ACQUIRING;
				pthread_cond_wait(&lock->lock_cond, &lock->lock_mutex);
				if(lock->stat == lock_info_client::FREE)goto lock_free;
				else {
					continue;
				}
			}
		}
		else if(lock->stat == lock_info_client::FREE){
			goto lock_free;
		}
		else {
			pthread_cond_wait(&lock->lock_cond, &lock->lock_mutex);
			if(lock->stat == lock_info_client::FREE)goto lock_free;
			else {
				continue;
			}
		}
	}

lock_free:
	lock->stat = lock_info_client::LOCKED;
	ret = lock_protocol::OK;
	pthread_mutex_unlock(&lock->lock_mutex);
	return ret;
}

lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid)
{
	lock_info_client* lock = get_lock(lid);
	pthread_mutex_lock(&lock->lock_mutex);
	if(lock->stat == lock_info_client::LOCKED){
		lock->stat = lock_info_client::FREE;
		pthread_cond_signal(&lock->lock_cond);
	}
	else{
		assert(lock->stat == lock_info_client::RELEASING);
		pthread_cond_signal(&lock->revoke_cond);
	}
	pthread_mutex_unlock(&lock->lock_mutex);
	return lock_protocol::OK;
}

