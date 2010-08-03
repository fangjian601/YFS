/*
 * chunk_protocol.h
 *
 *  Created on: 2010-7-30
 *      Author: frank
 */

#ifndef FILE_PROTOCOL_H_
#define FILE_PROTOCOL_H_

#include <string>
#include <list>
#include <sys/stat.h>

#include "rpc.h"

class file_protocol {
public:
	typedef unsigned long long file_t;
	typedef std::vector<std::pair<file_t, std::string> > entries_t;
	typedef unsigned int uint;

	struct metadata{
	    file_t  ino;
	    mode_t  mode;
	    nlink_t nlink;
	    uid_t   uid;
	    gid_t   gid;
	    uint    size;
	    uint	atime;
	    uint	mtime;
	    uint	ctime;

	    std::string filename;
	    int isdir;
	    std::list<file_t> subfiles;

	    metadata(file_t _fid = 0, std::string _filename = std::string(""));
		friend marshall & operator<<(marshall &m, file_protocol::metadata &s);
		friend unmarshall & operator>>(unmarshall &u, file_protocol::metadata &s);
	};
	enum xxstatus {
		OK, NOENT, NOTDIR, NOTREG, IOERR, RPCERR, EXIST, BUSY
	};
	typedef int status;
	enum rpc_numbers {
		read = 0x30001,
		write,
		create,
		unlink,
		truncate,
		flush
	};
};

inline file_protocol::metadata::metadata(file_protocol::file_t _fid, std::string _filename)
{
	ino = _fid;
	nlink = 0;
	gid = uid = 0;
	size = 0;
	atime = ctime = mtime = 0;
	filename = _filename;
	if(ino & 0x80000000){
		isdir = 0;
		mode = S_IFREG | 0x666;
	}
	else{
		isdir = 1;
		mode = S_IFDIR | 0x777;
	}
}

inline marshall & operator<<(marshall &m, file_protocol::metadata &s)
{
	m << s.ino;
	m << s.mode;
	m << s.nlink;
	m << s.gid;
	m << s.uid;
	m << s.size;
	m << s.atime;
	m << s.mtime;
	m << s.ctime;
	m << s.filename;
	m << s.isdir;
	m << s.subfiles.size();
	std::list<file_protocol::file_t>::iterator iter = s.subfiles.begin();
	for(; iter != s.subfiles.end(); ++iter){
		m << (*iter);
	}
	return m;
}

inline unmarshall & operator>>(unmarshall &u, file_protocol::metadata &s)
{
	u >> s.ino;
	u >> s.mode;
	u >> s.nlink;
	u >> s.gid;
	u >> s.uid;
	u >> s.size;
	u >> s.atime;
	u >> s.mtime;
	u >> s.ctime;
	u >> s.filename;
	u >> s.isdir;
	uint size;
	u >> size;
	for(uint i=0; i<size; i++){
		file_protocol::file_t subdir;
		u >> subdir;
		s.subfiles.push_back(subdir);
	}
	return u;
}

inline marshall & operator<<(marshall &m, file_protocol::entries_t &s){
	m << s.size();
	for(uint i=0; i<s.size(); i++){
		m << s[i].first;
		m << s[i].second;
	}
	return m;
}
inline unmarshall & operator>>(unmarshall &u, file_protocol::entries_t &s){
	uint size;
	u >> size;
	for(uint i=0; i<size; i++){
		std::pair<file_protocol::file_t, std::string> entry;
		u >> entry.first;
		u >> entry.second;
		s.push_back(entry);
	}
	return u;
}

inline marshall & operator<<(marshall &m, std::vector<file_protocol::file_t> &s){
	m << s.size();
	for(uint i=0; i<s.size(); i++){
		m << s[i];
	}
	return m;
}

inline unmarshall & operator>>(unmarshall &u, std::vector<file_protocol::file_t> &s){
	uint size;
	u >> size;
	for(uint i=0; i<size; i++){
		file_protocol::file_t file;
		u >> file;
		s.push_back(file);
	}
	return u;
}

#endif /* FILE_PROTOCOL_H_ */
