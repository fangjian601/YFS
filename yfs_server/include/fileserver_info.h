/*
 * fileserver_info.h
 *
 *  Created on: 2010-7-31
 *      Author: frank
 */

#ifndef FILESERVER_INFO_H_
#define FILESERVER_INFO_H_

#include <string>

#include "master.h"
#include "file_protocol.h"
#include "rpc.h"

class master;

class fileserver_info{
private:
	std::string id;
	std::vector<file_protocol::file_t> files;
	master* m;
	bool alive;
	pthread_t heartbeat_thread;
	pthread_mutex_t heartbeat_mutex;
	pthread_cond_t heartbeat_cond;
	rpcc* fc;
	static const unsigned int timeout = 10;
public:
	fileserver_info(std::string id, std::vector<file_protocol::file_t> _files, master* _m);
	std::string get_id();
	rpcc* get_rpcc();
	std::vector<file_protocol::file_t> get_files();
	void set_alive(bool _alive);
	void heartbeater();
	friend class master;
};

#endif /* FILESERVER_INFO_H_ */
