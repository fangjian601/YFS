/*
 * chunkserver.cc
 *
 *  Created on: 2010-7-30
 *      Author: frank
 */
#include <sstream>

#include "fileserver.h"

static void *
heartbeatthread(void *x){
	fileserver* fs = (fileserver*)x;
	fs->doheartbeat();
	return 0;
}

int fileserver::last_port = 0;

fileserver::fileserver(std::string xdst, std::string _root_path){
	root_path = _root_path;
	rcl = new rsm_client(xdst);
	insync = false;
	fs_init();
	fs_reg();
	fs_thread();
}

void
fileserver::fs_init(){
	srand(time(NULL) ^ last_port);
	port = ((rand() % 32000) | (0x1 << 10));
	hostname = "127.0.0.1";
	std::stringstream sstr;
	sstr << hostname << ":" << port;
	id = sstr.str();
	last_port = port;
	fs = new rpcs(port);
}

void
fileserver::fs_reg(){
	fs->reg(file_protocol::read, this, &fileserver::read);
	fs->reg(file_protocol::write, this, &fileserver::write);
	fs->reg(file_protocol::flush, this, &fileserver::flush);
	fs->reg(file_protocol::create, this, &fileserver::create);
	fs->reg(file_protocol::unlink, this, &fileserver::unlink);
}

void
fileserver::fs_thread(){
	pthread_create(&heartbeat_thread, NULL, &heartbeatthread, (void *)this);
	pthread_mutex_init(&heartbeat_mutex, NULL);
	pthread_mutex_init(&files_mutex, NULL);
	pthread_cond_init(&heartbeat_cond, NULL);
}

file_protocol::status
fileserver::read(file_protocol::file_t ino, uint size, uint off, std::string& buf){
	file_protocol::status ret = file_protocol::OK;
	return ret;
}
file_protocol::status
fileserver::write(file_protocol::file_t ino, std::string buf, uint size, uint off, int&){
	file_protocol::status ret = file_protocol::OK;
	return ret;
}
file_protocol::status
fileserver::flush(file_protocol::file_t ino, std::string buf, uint size, uint off, int&){
	file_protocol::status ret = file_protocol::OK;
	return ret;
}
file_protocol::status
fileserver::create(file_protocol::file_t ino, int&){
	file_protocol::status ret = file_protocol::OK;
	return ret;
}
file_protocol::status
fileserver::unlink(file_protocol::file_t ino, int&){
	file_protocol::status ret = file_protocol::OK;
	return ret;
}

void
fileserver::doheartbeat(){

}
