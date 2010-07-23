// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

lock_info_server::lock_info_server(lock_protocol::lockid_t _id)
{
	id = _id;
	stat = FREE;
	pthread_mutex_init(&lock_mutex, NULL);
	owner = NULL;
}

client_info::client_info(std::string _host, int _port){
	host = _host;
	port = _port;
	std::stringstream sstr;
	sstr << host << ":" << port;
	id = sstr.str();
	sockaddr_in dstsock;
	make_sockaddr(id.c_str(), &dstsock);
	cl = new rpcc(dstsock);
	if(cl->bind() < 0){
		printf("lock_server_cache: call bind\n");
	}

}

static void *
revokethread(void *x)
{
  lock_server_cache *sc = (lock_server_cache *) x;
  sc->revoker();
  return 0;
}

static void *
retrythread(void *x)
{
  lock_server_cache *sc = (lock_server_cache *) x;
  sc->retryer();
  return 0;
}

lock_server_cache::lock_server_cache(class rsm *_rsm) 
  : rsm (_rsm)
{
	pthread_mutex_init(&locks_mutex, NULL);
	pthread_mutex_init(&revoker_mutex, NULL);
	pthread_mutex_init(&retryer_mutex, NULL);
	pthread_cond_init(&revoker_cond, NULL);
	pthread_cond_init(&retryer_cond, NULL);
	pthread_t th;
	int r = pthread_create(&th, NULL, &revokethread, (void *) this);
	assert (r == 0);
	r = pthread_create(&th, NULL, &retrythread, (void *) this);
	assert (r == 0);
}

lock_server_cache::~lock_server_cache(){}

void
lock_server_cache::revoker()
{

	// This method should be a continuous loop, that sends revoke
	// messages to lock holders whenever another client wants the
	// same lock
	while(true){
		pthread_mutex_lock(&revoker_mutex);
		pthread_cond_wait(&revoker_cond, &revoker_mutex);
		if(rsm->amiprimary()){
			int size = revoke_list.size();
			for(int i=0; i<size; i++){
				request req = revoke_list.front();
				revoke_list.pop_front();
				int r;
				req.requester->cl->call(rlock_protocol::revoke, req.request_lid, r);
			}
		}
		else{
			revoke_list.clear();
		}
		pthread_mutex_unlock(&revoker_mutex);

	}

}


void
lock_server_cache::retryer()
{

	// This method should be a continuous loop, waiting for locks
	// to be released and then sending retry messages to those who
	// are waiting for it.
	while(true){
		pthread_mutex_lock(&retryer_mutex);
		pthread_cond_wait(&retryer_cond, &retryer_mutex);
		if(rsm->amiprimary()){
			int size = retry_list.size();
			for(int i=0; i<size; i++){
				request req = retry_list.front();
				retry_list.pop_front();
				int r;
				req.requester->cl->call(rlock_protocol::retry, req.request_lid, r);
			}
		}
		else{
			retry_list.clear();
		}
		pthread_mutex_unlock(&retryer_mutex);
	}
}

lock_protocol::status
lock_server_cache::stat(lock_protocol::lockid_t tid, int &)
{
	return lock_protocol::OK;
}
lock_protocol::status
lock_server_cache::acquire(std::string id, request_t rid, lock_protocol::lockid_t lid, int &)
{
	pthread_mutex_lock(&locks_mutex);
	lock_info_server* lock = get_lock(lid);
	lock_protocol::status ret = lock_protocol::OK;
	if(clients.find(id) == clients.end()){
		assert(true);
	}
	if(lock->stat == lock_info_server::FREE ||
	  (lock->stat == lock_info_server::RETRYING &&
	   lock->retry != NULL &&
	   lock->retry->id == id)){

		if(lock->stat != lock_info_server::FREE)lock->waiters.pop_front();
		lock->owner = clients[id];
		lock->stat = lock_info_server::LOCKED;
		if(lock->waiters.size() > 0){
			lock->stat = lock_info_server::REVOKING;
			pthread_mutex_lock(&revoker_mutex);
			request req;
			req.request_lid = lid;
			req.requester = lock->owner;
			revoke_list.push_back(req);
			pthread_cond_signal(&revoker_cond);
			pthread_mutex_unlock(&revoker_mutex);
		}
		ret = lock_protocol::OK;
	}
	else{
		lock->waiters.push_back(clients[id]);
		if(lock->stat == lock_info_server::LOCKED){
			lock->stat = lock_info_server::REVOKING;
			pthread_mutex_lock(&revoker_mutex);
			request req;
			req.request_lid = lid;
			req.requester = lock->owner;
			revoke_list.push_back(req);
			pthread_cond_signal(&revoker_cond);
			pthread_mutex_unlock(&revoker_mutex);
		}
		ret = lock_protocol::RETRY;
	}
	pthread_mutex_unlock(&locks_mutex);
	return ret;
}
lock_protocol::status
lock_server_cache::release(std::string id, request_t rid, lock_protocol::lockid_t lid, int &)
{
	pthread_mutex_lock(&locks_mutex);
	lock_info_server* lock = get_lock(lid);
	lock_protocol::status ret = lock_protocol::OK;
	lock->stat = lock_info_server::FREE;
	if(lock->waiters.size() > 0){
		client_info* waiter = lock->waiters.front();
		lock->retry = waiter;
		lock->stat = lock_info_server::RETRYING;
		pthread_mutex_lock(&retryer_mutex);
		request req;
		req.request_lid = lid;
		req.requester = waiter;
		retry_list.push_back(req);
		pthread_cond_signal(&retryer_cond);
		pthread_mutex_unlock(&retryer_mutex);
	}
	pthread_mutex_unlock(&locks_mutex);
	return ret;
}
lock_protocol::status
lock_server_cache::subscribe(std::string hostname, int port, int &)
{
	std::stringstream sstr;
	sstr << hostname << ":" << port;
	std::string id = sstr.str();
	assert(clients.find(id) == clients.end());
	clients[id] = new client_info(hostname, port);
	return lock_protocol::OK;
}

lock_info_server*
lock_server_cache::get_lock(lock_protocol::lockid_t lid)
{
	if(locks.find(lid) == locks.end()){
		lock_info_server* lock = new lock_info_server(lid);
		locks[lid] = lock;
	}
	return locks[lid];
}

std::string lock_server_cache::marshal_state()
{
	return "";
}

void lock_server_cache::unmarshal_state(std::string state)
{

}
