// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include "lock_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
	ec = new extent_client(extent_dst);
	lock_releaser* lr = new lock_releaser(ec);
	lc = new lock_client_cache(lock_dst,lr);

}

yfs_client::inum
yfs_client::n2i(std::string n)
{
	std::istringstream ist(n);
	unsigned long long finum;
	ist >> finum;
	return finum;
}

std::string
yfs_client::filename(inum inum)
{
	std::ostringstream ost;
	ost << inum;
	return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
	if(inum & 0x80000000)
		return true;
	return false;
}

bool
yfs_client::isdir(inum inum)
{
	return ! isfile(inum);
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
	int r = OK;

	lc->acquire(inum);

	printf("getfile %016llx\n", inum);
	extent_protocol::attr a;
	if (ec->getattr(inum, a) != extent_protocol::OK) {
		r = IOERR;
		goto release;
	}

	fin.atime = a.atime;
	fin.mtime = a.mtime;
	fin.ctime = a.ctime;
	fin.size = a.size;
	printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:
	lc->release(inum);
	return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
	int r = OK;

	lc->acquire(inum);

	printf("getdir %016llx\n", inum);
	extent_protocol::attr a;
	if (ec->getattr(inum, a) != extent_protocol::OK) {
		r = IOERR;
		goto release;
	}
	din.atime = a.atime;
	din.mtime = a.mtime;
	din.ctime = a.ctime;

release:
	lc->release(inum);
	return r;
}

yfs_client::inum yfs_client::ilookup(inum di, std::string name){
	if(isfile(di))return 0;
	std::string buf;
	get(di,buf);

	printf("yfs_client::ilookup: inum 0x%x, buf\n%s\n", (unsigned int)di, buf.c_str());

	std::vector<std::string> entries = yfs_client::split(buf,"\n",true,false);
	for(unsigned int i = 0; i < entries.size(); i++){
		std::string entry = entries[i];
		std::vector<std::string> info = yfs_client::split(entry," ", true,false);
		if(info.size() != 2)return 0;
		inum temp_inum = n2i(info[0]);
		std::string temp_name = info[1];
		if(temp_name == name) return temp_inum;
		else continue;
	}
	return 0;

}

int yfs_client::get(inum i, std::string& buf){
	extent_protocol::status ret = ec->get(i,buf);
	if(ret == extent_protocol::OK){
		return OK;
	}
	else if(ret == extent_protocol::NOENT) return NOENT;
	else return IOERR;
}

int yfs_client::put(inum i, std::string buf){
	extent_protocol::status ret = ec->put(i,buf);
	if(ret == extent_protocol::OK){
		return OK;
	}
	else return IOERR;
}

int yfs_client::getattr(inum eid, extent_protocol::attr& a){
	extent_protocol::status ret = ec->getattr(eid,a);
	if(ret == extent_protocol::OK){
		return OK;
	}
	else if(ret == extent_protocol::NOENT) return NOENT;
	else return IOERR;
}

int yfs_client::putattr(inum eid, extent_protocol::attr a){
	extent_protocol::status ret = ec->putattr(eid,a);
	if(ret == extent_protocol::OK){
		return OK;
	}
	else if(ret == extent_protocol::NOENT) return NOENT;
	else return IOERR;
}

int yfs_client::remove(inum i){
	extent_protocol::status ret = ec->remove(i);
	if(ret == extent_protocol::OK){
		return OK;
	}
	else if(ret == extent_protocol::NOENT) return NOENT;
	else return IOERR;
}

bool yfs_client::exist(inum id){
	int ret = 0;
	ec->exist(id,ret);
	if(ret == 0){

		return false;
	}
	else return true;
}

void yfs_client::acquire(inum eid){
	lc->acquire(eid);
	printf("yfs_client::acquire: acquired lock 0x%x\n", (unsigned int)eid);
}

void yfs_client::release(inum eid){
	lc->release(eid);
	printf("yfs_client::release: released lock 0x%x\n", (unsigned int)eid);
}

std::vector<std::string> yfs_client::split(const std::string& s, const std::string& match,
		bool removeEmpty,bool fullMatch){
	std::vector<std::string> result;
	std::string::size_type start = 0, skip = 1;
	find_t pfind = &std::string::find_first_of;

	if (fullMatch){
	   skip = match.length();
	   pfind = &std::string::find;
	}

	while (start != std::string::npos){
		std::string::size_type end = (s.*pfind)(match, start);
	   if (skip == 0) end = std::string::npos;

	   std::string token = s.substr(start, end - start);

	   if (!(removeEmpty && token.empty())){
		   result.push_back(token);
	   }
	   if ((start = end) != std::string::npos) start += skip;
	}

	return result;
}

