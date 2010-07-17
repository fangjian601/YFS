#ifndef yfs_client_h
#define yfs_client_h

#include <string>
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>

#include "lock_protocol.h"
#include "lock_client.h"
#include "lock_client_cache.h"

class yfs_client {
	extent_client *ec;
	lock_client_cache * lc;
public:

	typedef unsigned long long inum;
	enum xxstatus { OK, RPCERR, NOENT, IOERR, FBIG, EXIST};
	typedef int status;

	struct fileinfo {
		unsigned long long size;
		unsigned long atime;
		unsigned long mtime;
		unsigned long ctime;
	};
	struct dirinfo {
		unsigned long atime;
		unsigned long mtime;
		unsigned long ctime;
	};
	struct dirent {
		std::string name;
		unsigned long long inum;
	};

public:
	static std::string filename(inum);
	static inum n2i(std::string);
public:

	yfs_client(std::string, std::string);

	bool isfile(inum);
	bool isdir(inum);
	inum ilookup(inum di, std::string name);

	int getfile(inum, fileinfo &);
	int getdir(inum, dirinfo &);

	int get(inum, std::string& buf);
	int put(inum, std::string buf);
	int getattr(inum eid, extent_protocol::attr& a);
	int putattr(inum eid, extent_protocol::attr a);
	int remove(inum);
	bool exist(inum);

	void acquire(inum);
	void release(inum);

	typedef std::string::size_type (std::string::*find_t)(const std::string& delim, std::string::size_type offset) const;
	static std::vector<std::string> split(const std::string& s, const std::string& match, bool removeEmpty,bool fullMatch);
};

#endif 
