#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>
#include <map>
#include <list>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_server.h"

struct client_info{
	std::string id;
	std::string host;
	int port;
	rpcc* cl;
	client_info(std::string, int);
};

struct request{
	client_info* requester;
	lock_protocol::lockid_t request_lid;
};

struct lock_info_server{
	enum status{FREE,LOCKED,REVOKING,RETRYING};
	lock_protocol::lockid_t id;
	status stat;
	pthread_mutex_t lock_mutex;
	client_info* retry;
	client_info* owner;
	std::list<client_info*> waiters;
	lock_info_server(lock_protocol::lockid_t);
};

class lock_server_cache {
private:
	std::map<std::string, client_info*> clients;
	std::map<lock_protocol::lockid_t, lock_info_server*> locks;

	std::list<request> revoke_list;
	std::list<request> retry_list;

	pthread_mutex_t revoker_mutex;
	pthread_mutex_t retryer_mutex;
	pthread_mutex_t locks_mutex;

	pthread_cond_t revoker_cond;
	pthread_cond_t retryer_cond;

	lock_info_server* get_lock(lock_protocol::lockid_t);

public:
	lock_server_cache();
	lock_protocol::status stat(lock_protocol::lockid_t, int &);
	lock_protocol::status acquire(std::string id, lock_protocol::lockid_t, int &);
	lock_protocol::status release(std::string id, lock_protocol::lockid_t, int &);
	lock_protocol::status subscribe(std::string hostname, int port, int &);
	void revoker();
	void retryer();
};

#endif
