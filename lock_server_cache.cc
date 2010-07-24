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
}

marshall&
operator<<(marshall &m, struct lock_info_server &s)
{
	m<<s.id;
	m<<s.stat;
	m<<s.owner;
	m<<s.retry;
	m<<s.waiters.size();
	for(std::list<client_info>::iterator iter = s.waiters.begin();
			iter != s.waiters.end(); ++iter){
		m<<(*iter);
	}
	return m;
}

unmarshall&
operator>>(unmarshall &u, struct lock_info_server &s)
{
	u>>s.id;
	u>>s.stat;
	u>>s.owner;
	u>>s.retry;
	unsigned int size;
	u>>size;
	for(unsigned int i=0; i<size; i++){
		client_info cinfo;
		u>>cinfo;
		s.waiters.push_back(cinfo);
	}
	return u;
}
client_info::client_info(std::string _host, int _port){
	host = _host;
	port = _port;
	std::stringstream sstr;
	sstr << host << ":" << port;
	id = sstr.str();
}

marshall&
operator<<(marshall &m, struct client_info &s)
{
	m<<s.port;
	m<<s.host;
	return m;
}

unmarshall&
operator>>(unmarshall &u, struct client_info &s)
{
	std::string host;
	int port;
	u>>port;
	u>>host;
	s = client_info(host,port);
	return u;
}

marshall&
operator<<(marshall &m, struct request &s)
{
	m<<s.rid;
	m<<s.request_lid;
	m<<s.requester;
	return m;
}

unmarshall&
operator>>(unmarshall &u, struct request &s)
{
	u>>s.rid;
	u>>s.request_lid;
	u>>s.requester;
	return u;
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
	pthread_mutex_init(&clients_mutex, NULL);
	pthread_mutex_init(&revoker_mutex, NULL);
	pthread_mutex_init(&retryer_mutex, NULL);
	pthread_cond_init(&revoker_cond, NULL);
	pthread_cond_init(&retryer_cond, NULL);
	rsm->set_state_transfer(this);
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
				int r;
				if(get_rpcc(req.requester)->call(rlock_protocol::revoke, req.request_lid, r)
						== rlock_protocol::OK){
					revoke_list.pop_front();
				}
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
				int r;
				if(get_rpcc(req.requester)->call(rlock_protocol::retry, req.request_lid, r)
						== rlock_protocol::OK){
					retry_list.pop_front();
				}
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
	printf("lock_server_cache::acquire: id %s, rid %llu, lid %llu\n", id.c_str(), rid, lid);
	/*if(!rsm->amiprimary_wo()){
		pthread_mutex_lock(&retryer_mutex);
		std::list<request>::iterator iter = retry_list.begin();
		for(; iter != retry_list.end(); ++iter){
			if((*iter).request_lid == lid){
				retry_list.erase(iter);
				break;
			}
		}
		pthread_mutex_unlock(&retryer_mutex);
	}*/
	pthread_mutex_lock(&locks_mutex);
	lock_protocol::status ret = lock_protocol::OK;
	if(rsm->amiprimary_wo()&&
	   rpc_status.find(id) != rpc_status.end()&&
	   rpc_status[id].find(rid) != rpc_status[id].end()){
		set_lock(lid, rpc_status[id][rid]);
	}
	lock_info_server lock = get_lock(lid);
	rpc_status[id][rid] = lock;
	if(lock.stat == lock_info_server::FREE ||
	  (lock.stat == lock_info_server::RETRYING &&
	   lock.retry.id == id)){

		if(lock.stat != lock_info_server::FREE)lock.waiters.pop_front();
		lock.owner = get_client(id);
		lock.stat = lock_info_server::LOCKED;
		if(lock.waiters.size() > 0){
			lock.stat = lock_info_server::REVOKING;
			pthread_mutex_lock(&revoker_mutex);
			request req;
			req.request_lid = lid;
			req.requester = lock.owner;
			req.rid = rid;
			addrequest(req, revoke_list);
			pthread_cond_signal(&revoker_cond);
			pthread_mutex_unlock(&revoker_mutex);
		}
		ret = lock_protocol::OK;
	}
	else{
		lock.waiters.push_back(get_client(id));
		if(lock.stat == lock_info_server::LOCKED){
			lock.stat = lock_info_server::REVOKING;
			pthread_mutex_lock(&revoker_mutex);
			request req;
			req.request_lid = lid;
			req.requester = lock.owner;
			req.rid = rid;
			addrequest(req, revoke_list);
			pthread_cond_signal(&revoker_cond);
			pthread_mutex_unlock(&revoker_mutex);
		}
		ret = lock_protocol::RETRY;
	}
	set_lock(lid, lock);
	pthread_mutex_unlock(&locks_mutex);
	return ret;
}
lock_protocol::status
lock_server_cache::release(std::string id, request_t rid, lock_protocol::lockid_t lid, int &)
{
	printf("lock_server_cache::release: id %s, rid %llu, lid %llu\n", id.c_str(), rid, lid);
	/*if(!rsm->amiprimary_wo()){
		pthread_mutex_lock(&revoker_mutex);
		std::list<request>::iterator iter = revoke_list.begin();
		for(; iter != revoke_list.end(); ++iter){
			if((*iter).request_lid == lid){
				revoke_list.erase(iter);
				break;
			}
		}
		pthread_mutex_unlock(&revoker_mutex);
	}*/
	pthread_mutex_lock(&locks_mutex);
	lock_protocol::status ret = lock_protocol::OK;
	if(rsm->amiprimary_wo()&&
	   rpc_status.find(id) != rpc_status.end()&&
	   rpc_status[id].find(rid) != rpc_status[id].end()){
		set_lock(lid,rpc_status[id][rid]);
	}
	lock_info_server lock = get_lock(lid);
	rpc_status[id][rid] = lock;
	lock.stat = lock_info_server::FREE;
	if(lock.waiters.size() > 0){
		client_info waiter = lock.waiters.front();
		lock.retry = waiter;
		lock.stat = lock_info_server::RETRYING;
		pthread_mutex_lock(&retryer_mutex);
		request req;
		req.request_lid = lid;
		req.requester = waiter;
		req.rid = rid;
		addrequest(req, retry_list);
		pthread_cond_signal(&retryer_cond);
		pthread_mutex_unlock(&retryer_mutex);
	}
	set_lock(lid, lock);
	pthread_mutex_unlock(&locks_mutex);
	return ret;
}
lock_protocol::status
lock_server_cache::subscribe(std::string hostname, int port, int &)
{
	std::stringstream sstr;
	sstr << hostname << ":" << port;
	std::string id = sstr.str();
	pthread_mutex_lock(&clients_mutex);
	assert(clients.find(id) == clients.end());
	clients[id] = client_info(hostname, port);
	pthread_mutex_unlock(&clients_mutex);
	return lock_protocol::OK;
}

void
lock_server_cache::addrequest(request req, std::list<request>& req_list)
{
	bool exist = false;
	for(std::list<request>::iterator iter = req_list.begin();
			iter != req_list.end(); ++iter){
		request tmp = *iter;
		if(tmp.rid == req.rid && tmp.requester.id == req.requester.id){
			exist = true;
			break;
		}
	}
	if(!exist){
		req_list.push_back(req);
	}
}

lock_info_server
lock_server_cache::get_lock(lock_protocol::lockid_t lid)
{
	if(locks.find(lid) == locks.end()){
		lock_info_server lock = lock_info_server(lid);
		locks[lid] = lock;
	}
	return locks[lid];
}

client_info
lock_server_cache::get_client(std::string cid)
{
	return clients[cid];
}

rpcc*
lock_server_cache::get_rpcc(client_info client)
{

    if (rpcc_cache.find(client.id) == rpcc_cache.end()) {
		sockaddr_in dstsock;
		make_sockaddr(client.id.c_str(), &dstsock);
		rpcc* cl = new rpcc(dstsock);
		if (cl->bind() < 0) {
			printf("lock_server_cache::revoker(): bind failed\n");
			return NULL;
		}
		rpcc_cache[client.id] = cl;
	}
    return rpcc_cache[client.id];
}

void
lock_server_cache::set_lock(lock_protocol::lockid_t lid, lock_info_server lock)
{
	locks[lid] = lock;
}

void
lock_server_cache::set_client(std::string cid, client_info client)
{
	pthread_mutex_lock(&clients_mutex);
	clients[cid] = client;
	pthread_mutex_unlock(&clients_mutex);
}

std::string lock_server_cache::marshal_state()
{
	marshall rep;
	pthread_mutex_lock(&clients_mutex);
	rep << clients.size();
	std::map<std::string, client_info>::iterator iter = clients.begin();
	for(; iter != clients.end(); ++iter){
		rep << iter->first;
		rep << iter->second;
	}
	pthread_mutex_unlock(&clients_mutex);

	pthread_mutex_lock(&locks_mutex);
	rep << locks.size();
	std::map<lock_protocol::lockid_t, lock_info_server>::iterator iter_locks = locks.begin();
	for(; iter_locks != locks.end(); ++iter_locks){
		rep << iter_locks->first;
		rep << iter_locks->second;
	}

	rep << rpc_status.size();
	std::map<std::string, std::map<request_t, lock_info_server> >::iterator iter_rpc = rpc_status.begin();
	for(; iter_rpc != rpc_status.end(); ++iter_rpc){
		rep << iter_rpc->first;
		std::map<request_t, lock_info_server> requests = iter_rpc->second;
		rep << requests.size();
		std::map<request_t, lock_info_server>::iterator iter_req = requests.begin();
		for(; iter_req != requests.end(); ++iter_req){
			rep << iter_req->first;
			rep << iter_req->second;
		}
	}
	pthread_mutex_unlock(&locks_mutex);

	pthread_mutex_lock(&retryer_mutex);
	rep << retry_list.size();
	std::list<request>::iterator iter_retry = retry_list.begin();
	for(; iter_retry != retry_list.end(); ++iter_retry){
		rep << (*iter_retry);
	}
	pthread_mutex_unlock(&retryer_mutex);

	pthread_mutex_lock(&revoker_mutex);
	rep << revoke_list.size();
	std::list<request>::iterator iter_revoke = revoke_list.begin();
	for(; iter_revoke != revoke_list.end(); ++iter){
		rep << (*iter_revoke);
	}
	pthread_mutex_unlock(&revoker_mutex);

	return rep.str();
}

void lock_server_cache::unmarshal_state(std::string state)
{
	unmarshall rep(state);
	unsigned int size;
	rep >> size;
	pthread_mutex_lock(&clients_mutex);
	for(unsigned int i = 0; i < size; i++){
		std::string cid;
		client_info cinfo;
		rep >> cid;
		rep >> cinfo;
		clients[cid] = cinfo;
	}
	pthread_mutex_unlock(&clients_mutex);

	rep >> size;
	pthread_mutex_lock(&locks_mutex);
	for(unsigned int i = 0; i < size; i++){
		lock_protocol::lockid_t lid;
		lock_info_server linfo;
		rep >> lid;
		rep >> linfo;
		locks[lid] = linfo;
	}

	rep >> size;
	for(unsigned int i = 0; i < size; i++){
		std::string id;
		rep >> id;
		unsigned int req_size;
		rep >> req_size;
		std::map<request_t, lock_info_server> requests;
		for(unsigned int j = 0; j < req_size; j++){
			request_t req;
			lock_info_server lock;
			rep >> req;
			rep >> lock;
			requests[req] = lock;
		}
		rpc_status[id] = requests;
	}
	pthread_mutex_unlock(&locks_mutex);

	rep >> size;
	pthread_mutex_lock(&retryer_mutex);
	for(unsigned int i = 0; i < size; i++){
		request req;
		rep >> req;
		retry_list.push_back(req);
	}
	pthread_mutex_unlock(&retryer_mutex);

	rep >> size;
	pthread_mutex_lock(&revoker_mutex);
	for(unsigned int i = 0; i < size; i++){
		request req;
		rep >> req;
		revoke_list.push_back(req);
	}
	pthread_mutex_unlock(&revoker_mutex);
}
