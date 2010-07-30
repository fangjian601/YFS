/*
 * chunk_protocol.h
 *
 *  Created on: 2010-7-30
 *      Author: frank
 */

#ifndef CHUNK_PROTOCOL_H_
#define CHUNK_PROTOCOL_H_

#include <string>
#include <vector>
#include <list>

#include "rpc.h"

#define CHUNK_SIZE 4096

class chunk_protocol {
public:
	typedef unsigned long long chunk_t;
	typedef unsigned long long file_t;
	typedef unsigned int uint;

	struct chunk {
		chunk_t cid;
		uint size;
		int removed;
		file_t fid;
		std::string primary;
		std::vector<std::string> replicates;

		chunk(chunk_protocol::chunk_t _cid, chunk_protocol::file_t _fid = 0,
				uint _size = 0, std::string _primary = std::string(""));
		~chunk();
		friend marshall & operator<<(marshall &m, chunk_protocol::chunk &s);
		friend unmarshall & operator>>(unmarshall &u, chunk_protocol::chunk &s);
	};

	struct file {
		file_t fid;
		std::string name;
		std::string path;
		std::list<chunk_t> chunks;

		file(chunk_protocol::file_t _fid, std::string _name, std::string _path);
		~file();
		friend marshall & operator<<(marshall &m, file &s);
		friend unmarshall & operator>>(unmarshall &u, file &s);
	};

	enum xxstatus {
		OK, NOENT, IOERR, RPCERR, EXIST, BUSY
	};
	typedef int status;
	enum rpc_numbers {
		read,
		write,
		create,
		getattr,
		setattr,
		getdir,
		mkdir,
		unlink,
		rmdir,
		rename,
		chmod,
		chown,
		truncate,
		flush
	};
};

chunk_protocol::chunk::chunk(chunk_protocol::chunk_t _cid, file_t _fid,
		uint _size, std::string _primary) {
	cid = _cid;
	fid = _fid;
	size = _size;
	primary = _primary;
	removed = false;
}

chunk_protocol::chunk::~chunk() {
}

chunk_protocol::file::file(chunk_protocol::file_t _fid, std::string _name, std::string _path){
	fid = _fid;
	name = _name;
	path = _path;
}

chunk_protocol::file::~file(){}

marshall & operator<<(marshall &m, chunk_protocol::chunk &s) {
	m << s.cid;
	m << s.fid;
	m << s.size;
	m << s.removed;
	m << s.primary;
	m << s.replicates.size();
	for (unsigned int i = 0; i < s.replicates.size(); i++) {
		m << s.replicates[i];
	}
	return m;
}

unmarshall & operator>>(unmarshall &u, chunk_protocol::chunk &s) {
	u >> s.cid;
	u >> s.cid;
	u >> s.fid;
	u >> s.size;
	u >> s.removed;
	u >> s.primary;
	unsigned int repli_size;
	u >> repli_size;
	for (unsigned int i = 0; i < repli_size; i++) {
		std::string repli;
		u >> repli;
		s.replicates.push_back(repli);
	}
	return u;
}

#endif /* CHUNK_PROTOCOL_H_ */
