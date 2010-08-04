/*
 * master.h
 *
 *  Created on: 2010-7-30
 *      Author: frank
 */

#ifndef MASTER_H_
#define MASTER_H_

#include <map>
#include <set>

#include "rsm.h"
#include "rsm_state_transfer.h"
#include "rpc.h"
#include "file_protocol.h"
#include "master_protocol.h"
#include "fileserver_info.h"

class fileserver_info;

class master_protocol{
public:
	enum xxstatus{OK, ERR, NOENT, EXIST, BUSY};
	typedef int status;
	enum rpc_numbers{
		add_fileserver = 0x40001,
		remove_fileserver,
		heartbeat
	};
};

class master : public rsm_state_transfer{
private:
	rsm* rsm_server;
	std::map<file_protocol::file_t, file_protocol::metadata> files;
	std::map<file_protocol::file_t, std::set<std::string> > replicates;
	std::map<std::string, fileserver_info*> fileservers;
	std::map<std::string, rpcc*> rpcc_cache;

	pthread_mutex_t fileservers_mutex;
	pthread_mutex_t files_mutex;

	master_protocol::status add_fileserver(std::string id,
										   std::vector<file_protocol::file_t> files, int&);
	master_protocol::status remove_fileserver(std::string id, int &);
	master_protocol::status heartbeat(std::string id, int &);

	master_client_protocol::status read(file_protocol::file_t ino,
										std::vector<std::string>& reps);
	master_client_protocol::status write(file_protocol::file_t ino,
										 std::vector<std::string>& reps);
	master_client_protocol::status flush(file_protocol::file_t ino,
										 std::vector<std::string>& reps);
	master_client_protocol::status create(file_protocol::file_t parent, std::string name,
										  mode_t mode, std::vector<std::string>& reps);
	master_client_protocol::status unlink(file_protocol::file_t parent,
										  std::string name, std::vector<std::string>& reps);
	master_client_protocol::status open(file_protocol::file_t ino, int &);
	master_client_protocol::status getattr(file_protocol::file_t ino,
										   file_protocol::metadata& attr);
	master_client_protocol::status setattr(file_protocol::file_t ino ,
										   file_protocol::metadata attr, int &);
	master_client_protocol::status readlink(file_protocol::file_t ino, std::string& link);
	master_client_protocol::status link(file_protocol::file_t parent, std::string name,
										file_protocol::metadata& attr);
	master_client_protocol::status symlink(file_protocol::file_t parent, std::string name,
										   file_protocol::metadata& attr);
	master_client_protocol::status readdir(file_protocol::file_t ino,
										   file_protocol::entries_t& entries);
	master_client_protocol::status mkdir(file_protocol::file_t parent, std::string name,
										 mode_t mode, file_protocol::metadata& attr);
	master_client_protocol::status rmdir(file_protocol::file_t parent, std::string name, int &);
	master_client_protocol::status rename(file_protocol::file_t parent, std::string name,
										  file_protocol::file_t newparent, std::string newname,
										  int &);
	master_client_protocol::status access(file_protocol::file_t ino, int mask, int &);

	void delete_fileserver_wo(std::string id);
	rpcc* get_rpcc(std::string _id);
public:
	master(class rsm* _rsm = 0);
	virtual ~master();
	void delete_fileserver(std::string id);
	virtual std::string marshal_state();
	virtual void unmarshal_state(std::string);
};

#endif /* MASTER_H_ */
