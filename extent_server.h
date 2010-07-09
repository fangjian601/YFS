// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include "extent_protocol.h"

struct file_info{
	file_info(std::string buf, extent_protocol::attr a);
	std::string buffer;
	extent_protocol::attr attr;
};

class extent_server {

private:
	std::map<extent_protocol::extentid_t, file_info*> files;

public:
	extent_server();

	int put(extent_protocol::extentid_t id, std::string, int &);
	int get(extent_protocol::extentid_t id, std::string &);
	int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
	int remove(extent_protocol::extentid_t id, int &);
	int exist(extent_protocol::extentid_t id, int& isexist);
};

#endif 







