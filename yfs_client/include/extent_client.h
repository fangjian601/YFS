// extent client interface.

#ifndef extent_client_h
#define extent_client_h

#include <string>
#include "extent_protocol.h"
#include "rpc.h"

class cache_file{
public:
	std::string buffer;
	extent_protocol::attr attr;
	bool data_dirty;
	bool meta_dirty;
	bool data_cached;
	bool meta_cached;
	bool removed;
public:
	cache_file(std::string buf, extent_protocol::attr a,
			   bool _data_diry = false, bool _meta_dirty = false);
	cache_file(std::string buf, bool _data_dirty = false);
	cache_file(extent_protocol::attr a, bool _meta_diry = false);
};

class extent_client {
private:
	rpcc *cl;
	std::map<extent_protocol::extentid_t, cache_file*> cache_files;
	pthread_mutex_t cache_mutex;

public:
	extent_client(std::string dst);

	extent_protocol::status get(extent_protocol::extentid_t eid,
				  std::string &buf);
	extent_protocol::status putattr(extent_protocol::extentid_t eid, extent_protocol::attr attr);
	extent_protocol::status getattr(extent_protocol::extentid_t eid,
				  extent_protocol::attr &attr);
	extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);
	extent_protocol::status remove(extent_protocol::extentid_t eid);
	extent_protocol::status exist(extent_protocol::extentid_t eid, int& isexist);
	extent_protocol::status flush(extent_protocol::extentid_t eid);
};

#endif 

