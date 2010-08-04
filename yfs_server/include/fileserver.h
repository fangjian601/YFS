/*
 * chunkserver.h
 *
 *  Created on: 2010-7-30
 *      Author: frank
 */

#ifndef FILESERVER_H_
#define FILESERVER_H_

#include <string>
#include <map>


#include "rsm_client.h"
#include "file_protocol.h"
#include "master.h"

class fileserver{
private:
	std::string id;
	std::string root_path;
	std::map<file_protocol::file_t, std::vector<std::string> > files;
	pthread_mutex_t files_mutex;
	bool insync;
	pthread_t heartbeat_thread;
	pthread_mutex_t heartbeat_mutex;
	pthread_cond_t heartbeat_cond;
	rsm_client* rcl;
	rpcs* fs;
	std::map<std::string, rpcc*> rpcc_cache;

	static int last_port;
	int port;
	std::string hostname;

	void fs_init();
	void fs_reg();
	void fs_thread();
	rpcc* get_rpcc(std::string _id);

	file_protocol::status read(file_protocol::file_t, uint, uint, std::string&);
	file_protocol::status write(file_protocol::file_t, std::string, uint, uint, int&);
	file_protocol::status flush(file_protocol::file_t, std::string, uint, uint, int&);
	file_protocol::status create(file_protocol::file_t, int&);
	file_protocol::status unlink(file_protocol::file_t, int&);
public:
	fileserver(std::string xdst, std::string _root_path);
	~fileserver();
	void doheartbeat();
};


#endif /* FILESERVER_H_ */
