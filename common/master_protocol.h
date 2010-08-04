/*
 * master_protocol.h
 *
 *  Created on: 2010-7-30
 *      Author: frank
 */

#ifndef MASTER_PROTOCOL_H_
#define MASTER_PROTOCOL_H_

#define TIMEOUT 1000

class master_client_protocol{
public:
	enum xxstatus {
		OK, NOENT, IOERR, RPCERR, EXIST, BUSY
	};
	typedef int status;
	enum rpc_numbers{
		read = 0x20001,
		write,
		flush,
		open,
		readlink,
		symlink,
		link,
		getattr,
		setattr,
		readdir,
		mkdir,
		rmdir,
		rename,
		create,
		unlink,
		access
	};
};

#endif /* MASTER_PROTOCOL_H_ */
