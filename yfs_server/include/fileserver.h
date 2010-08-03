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


class fileserver{
private:
	std::string id;
	std::map<file_protocol::file_t, std::vector<std::string> > files;
	rsm_client* rcl;
	rpcs* fs;
	std::map<std::string, rpcc*> rpcc_cache;
};


#endif /* FILESERVER_H_ */
