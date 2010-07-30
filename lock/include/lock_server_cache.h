#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>
#include <map>
#include <list>
#include "lock_protocol.h"
#include "rpc.h"
#include "rsm.h"
#include "rsm_state_transfer.h"
struct client_info {
	std::string id;
	std::string host;
	int port;
	client_info(std::string _host = std::string(), int  _port = 0);
	friend marshall & operator<<(marshall &m, struct client_info &s);
	friend unmarshall & operator>>(unmarshall &u, struct client_info &s);
};

struct request {
	request_t rid;
	client_info requester;
	lock_protocol::lockid_t request_lid;
	friend marshall & operator<<(marshall &m, struct request &s);
	friend unmarshall & operator>>(unmarshall &u, struct request &s);
};

struct lock_info_server {
	enum status {
		FREE = 0, LOCKED = 1, REVOKING = 2, RETRYING = 3
	};
	lock_protocol::lockid_t id;
	int stat;
	client_info retry;
	client_info owner;
	std::list<client_info> waiters;
	lock_info_server(lock_protocol::lockid_t lid = 0);
	friend marshall & operator<<(marshall &m, struct lock_info_server &s);
	friend unmarshall & operator>>(unmarshall &u, struct lock_info_server &s);
};

class lock_server_cache: public rsm_state_transfer {
private:
	class rsm *rsm;
public:
	lock_server_cache(class rsm *rsm = 0);
	virtual ~lock_server_cache();
private:
	std::map<std::string, client_info> clients;
	std::map<lock_protocol::lockid_t, lock_info_server> locks;
	std::map<std::string, rpcc*> rpcc_cache;
	std::map<std::string, std::map<request_t, lock_info_server> > rpc_status;

	std::list<request> revoke_list;
	std::list<request> retry_list;

	pthread_mutex_t revoker_mutex;
	pthread_mutex_t retryer_mutex;
	pthread_mutex_t locks_mutex;
	pthread_mutex_t clients_mutex;

	pthread_cond_t revoker_cond;
	pthread_cond_t retryer_cond;

	void addrequest(request req, std::list<request>& req_list);
	lock_info_server get_lock(lock_protocol::lockid_t);
	client_info get_client(std::string cid);
	rpcc* get_rpcc(client_info);
	void set_lock(lock_protocol::lockid_t lid, lock_info_server lock);
	void set_client(std::string cid, client_info);

public:
	lock_server_cache();
	lock_protocol::status stat(lock_protocol::lockid_t, int &);
	lock_protocol::status acquire(std::string id, request_t rid,
			lock_protocol::lockid_t, int &);
	lock_protocol::status release(std::string id, request_t rid,
			lock_protocol::lockid_t, int &);
	lock_protocol::status subscribe(std::string hostname, int port, int &);
	void revoker();
	void retryer();
	virtual std::string marshal_state();
	virtual void unmarshal_state(std::string);
};

#endif
