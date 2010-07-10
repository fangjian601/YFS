// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include <map>
#include <pthread.h>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"

struct lock{
	lock(lock_protocol::lockid_t lid);
	~lock();
	lock_protocol::lockid_t id;
	bool granted;
};


class lock_server {

 protected:
  int nacquire;
  pthread_mutex_t mutex;
  std::map<lock_protocol::lockid_t, lock*> locks;

 public:
  typedef std::map<lock_protocol::lockid_t, lock*>::iterator LockIterator;
  lock_server();
  ~lock_server();
  lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status acquire(int clt, lock_protocol::lockid_t, int &);
  lock_protocol::status release(int clt, lock_protocol::lockid_t, int &);
};

#endif 







