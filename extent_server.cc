// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctime>


file_info::file_info(std::string buf, extent_protocol::attr a){
	buffer = buf;
	attr = a;
}

extent_server::extent_server() {
	std::string buf;
	int r;
	put(0x1,buf, r);
}


int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
	printf("extent_server::put id 0x%x buf %s\n", (unsigned int)id, buf.c_str());
	time_t current_time;
	time(&current_time);
	if(files.find(id) == files.end()){
		extent_protocol::attr a;
		a.atime = current_time;
		a.ctime = current_time;
		a.mtime = current_time;
		a.size = buf.size();
		file_info* newFile = new file_info(buf,a);
		files[id] = newFile;
	}
	else{
		files[id]->attr.mtime = current_time;
		files[id]->attr.ctime = current_time;
		files[id]->attr.size = buf.size();
		files[id]->buffer = buf;
	}
	return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
	printf("extent_server::get id 0x%x\n", (unsigned int)id);
	time_t current_time;
	time(&current_time);
	if(files.find(id) == files.end()){
		return extent_protocol::IOERR;
	}
	else{
		files[id]->attr.atime = current_time;
		buf = files[id]->buffer;
	}
	return extent_protocol::OK;
}

int extent_server::putattr(extent_protocol::extentid_t id, extent_protocol::attr a, int &){
	if(files.find(id) == files.end()){
		return extent_protocol::IOERR;
	}
	files[id]->attr = a;
	return extent_protocol::OK;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
	// You replace this with a real implementation. We send a phony response
	// for now because it's difficult to get FUSE to do anything (including
	// unmount) if getattr fails.
	if(files.find(id) == files.end()){
		return extent_protocol::IOERR;
	}
	a.size = files[id]->attr.size;
	a.atime = files[id]->attr.atime;
	a.mtime = files[id]->attr.mtime;
	a.ctime = files[id]->attr.ctime;
	return extent_protocol::OK;
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
	if(files.find(id) == files.end()){
		return extent_protocol::IOERR;
	}
	files.erase(id);
	return extent_protocol::OK;
}

int extent_server::exist(extent_protocol::extentid_t id, int& isexist){
	if(files.find(id) == files.end()){
		isexist = 0;
	}
	else isexist = 1;
	return extent_protocol::OK;
}
