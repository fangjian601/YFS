// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

lock::lock(lock_protocol::lockid_t lid){
	id = lid;
	granted = true;
}

lock::~lock(){}

lock_server::lock_server():
  nacquire (0)
{
	pthread_mutex_init(&mutex,NULL);
}

lock_server::~lock_server(){
	pthread_mutex_destroy(&mutex);
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
	pthread_mutex_lock(&mutex);
	lock_protocol::status ret = lock_protocol::OK;
	printf("stat request from clt %d\n", clt);
	r = nacquire;
	pthread_mutex_unlock(&mutex);
	return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r){
	pthread_mutex_lock(&mutex);
	lock_protocol::status ret = lock_protocol::OK;
	if(locks.find(lid) == locks.end()){
		locks[lid] = new lock(lid);
		ret = lock_protocol::OK;
	}
	else{
		if(locks[lid]->granted){
			ret = lock_protocol::RETRY;
		}
		else{
			locks[lid]->granted = true;
			ret = lock_protocol::OK;
		}
	}
	pthread_mutex_unlock(&mutex);
	return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r){
	pthread_mutex_lock(&mutex);
	lock_protocol::status ret = lock_protocol::OK;
	if(locks.find(lid) != locks.end()){
		if(!locks[lid]->granted){
			ret = lock_protocol::RETRY;
		}
		else{
			locks[lid]->granted = false;
			ret = lock_protocol::OK;
		}
	}
	pthread_mutex_unlock(&mutex);
	return ret;
}

