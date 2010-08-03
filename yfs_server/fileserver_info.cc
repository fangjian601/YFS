/*
 * fileserver_info.cc
 *
 *  Created on: 2010-7-31
 *      Author: frank
 */

#include <stdio.h>

#include "fileserver_info.h"

static void *
heartbeatthread(void *x)
{
	fileserver_info *sc = (fileserver_info *) x;
	sc->heartbeater();
	return 0;
}

fileserver_info::fileserver_info(std::string _id, std::vector<file_protocol::file_t> _files,
							     master* _m){
	id = _id;
	files = _files;
	m = _m;
	sockaddr_in dstsock;
	make_sockaddr(id.c_str(), &dstsock);
	fc = new rpcc(dstsock);
	if(fc->bind() < 0){
		printf("fileserver_info::fileserver_info: fc bind error\n");
		fc = NULL;
	}
	pthread_mutex_init(&heartbeat_mutex, NULL);
	pthread_cond_init(&heartbeat_cond, NULL);
	pthread_create(&heartbeat_thread, NULL, &heartbeatthread, (void *)m);
}

void
fileserver_info::heartbeater(){
	timespec mytime;
	while(1){
		pthread_mutex_lock(&heartbeat_mutex);
		mytime.tv_sec = time(NULL) + timeout;
		mytime.tv_nsec = 0;
		pthread_cond_timedwait(&heartbeat_cond, &heartbeat_mutex, &mytime);
		if(alive){
			alive = false;
			continue;
		}
		else{
			m->delete_fileserver(id);
			break;
		}
		pthread_mutex_unlock(&heartbeat_mutex);
	}
}

std::string
fileserver_info::get_id(){
	return id;
}

rpcc*
fileserver_info::get_rpcc(){
	return fc;
}

std::vector<file_protocol::file_t>
fileserver_info::get_files(){
	return files;
}

void
fileserver_info::set_alive(bool _alive){
	alive = _alive;
}
