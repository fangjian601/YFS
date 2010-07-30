/*
 * master_protocol.h
 *
 *  Created on: 2010-7-30
 *      Author: frank
 */

#ifndef MASTER_PROTOCOL_H_
#define MASTER_PROTOCOL_H_

class master_client_protocol{
public:
	enum xxstatus{OK, ERR, BUSY};
	typedef int status;
	enum rpc_numbers{
		open,
		opendir
	};
};

#endif /* MASTER_PROTOCOL_H_ */
