/*
 * master.h
 *
 *  Created on: 2010-7-30
 *      Author: frank
 */

#ifndef MASTER_H_
#define MASTER_H_

#include <map>

#include "rsm.h"
#include "rpc.h"
#include "chunk_protocol.h"
#include "master_protocol.h"

class master_chunk_protocol{
public:
	enum xxstatus{OK, ERR, BUSY};
	typedef int status;
	enum rpc_numbers{
		add_chunk_server,
		remove_chunk_server,
		change_chunk_primary,
		change_chunk_replicates,
		chunk_heartbeatret
	};
};

class master{
private:
	rsm* rsm_server;
	std::map<chunk_protocol::chunk_t, chunk_protocol::chunk> chunks;
	std::map<chunk_protocol::file_t, chunk_protocol::file> files;
public:
	master(std::string primary, std::string me);
};

#endif /* MASTER_H_ */
