// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

cache_file::cache_file(std::string buf, extent_protocol::attr a,
					   bool _data_dirty, bool _meta_dirty)
: buffer(buf), attr(a), data_dirty(_data_dirty), meta_dirty(_meta_dirty),
  data_cached(true), meta_cached(true), removed(false){}

cache_file::cache_file(std::string buf, bool _data_dirty)
: buffer(buf), data_dirty(_data_dirty), meta_dirty(false),
  data_cached(true), meta_cached(false), removed(false){}

cache_file::cache_file(extent_protocol::attr a, bool _meta_dirty)
: attr(a), data_dirty(false), meta_dirty(_meta_dirty),
  data_cached(false), meta_cached(true), removed(false){}

// The calls assume that the caller holds a lock on the extent

extent_client::extent_client(std::string dst)
{
	sockaddr_in dstsock;
	make_sockaddr(dst.c_str(), &dstsock);
	cl = new rpcc(dstsock);
	if (cl->bind() != 0) {
		printf("extent_client: bind failed\n");
	}
	pthread_mutex_init(&cache_mutex, NULL);
}

extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
{
	pthread_mutex_lock(&cache_mutex);
	extent_protocol::status ret = extent_protocol::OK;
	time_t current;
	time(&current);
	if(cache_files.find(eid) == cache_files.end()){
		extent_protocol::attr a;
		ret = cl->call(extent_protocol::get, eid, buf);
		ret = cl->call(extent_protocol::getattr, eid, a);
		a.atime = current;
		cache_files[eid] = new cache_file(buf,a);
	}
	else if(cache_files[eid]->removed){
		return extent_protocol::NOENT;
	}
	else{
		cache_file* file = cache_files[eid];
		assert(file->meta_cached);
		if(!file->data_cached){
			ret = cl->call(extent_protocol::get, eid, buf);
			file->buffer = buf;
			file->data_cached = true;
		}
		else{
			buf = file->buffer;
		}
		file->attr.atime = current;
	}
	printf("extent_client::get eid 0x%x, buf %s\n", (unsigned int)eid, buf.c_str());
	pthread_mutex_unlock(&cache_mutex);
	return ret;
}

extent_protocol::status
extent_client::putattr(extent_protocol::extentid_t eid, extent_protocol::attr attr){
	printf("extent_client::putattr eid 0x%x\n", (unsigned int)eid);
	pthread_mutex_lock(&cache_mutex);
	extent_protocol::status ret = extent_protocol::OK;
	if(cache_files.find(eid) == cache_files.end()){
		int r;
		ret = cl->call(extent_protocol::putattr, eid, attr, r);
		if(ret == extent_protocol::OK)cache_files[eid] = new cache_file(attr);
	}
	else if(cache_files[eid]->removed){
		return extent_protocol::NOENT;
	}
	else{
		cache_file* file = cache_files[eid];
		assert(file->meta_cached);
		file->attr = attr;
		file->meta_cached = true;
		file->meta_dirty  = true;
	}
	pthread_mutex_unlock(&cache_mutex);
	return ret;
}

extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr &attr)
{
	printf("extent_client::getattr eid 0x%x\n", (unsigned int)eid);
	pthread_mutex_lock(&cache_mutex);
	extent_protocol::status ret = extent_protocol::OK;
	if(cache_files.find(eid) == cache_files.end()){
		ret = cl->call(extent_protocol::getattr, eid, attr);
		cache_files[eid] = new cache_file(attr);
	}
	else if(cache_files[eid]->removed){
		return extent_protocol::NOENT;
	}
	else{
		cache_file* file = cache_files[eid];
		assert(file->meta_cached);
		file->meta_dirty = true;
		attr = file->attr;
	}
	pthread_mutex_unlock(&cache_mutex);
	return ret;
}

extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf)
{
	printf("extent_client::put eid 0x%x, buf %s\n", (unsigned int)eid, buf.c_str());
	pthread_mutex_lock(&cache_mutex);
	extent_protocol::status ret = extent_protocol::OK;
	time_t current;
	time(&current);
	if(cache_files.find(eid) == cache_files.end()){
		extent_protocol::attr a;
		int r;
		ret = cl->call(extent_protocol::put, eid, buf, r);
		ret = cl->call(extent_protocol::getattr, eid, a);
		assert(ret == extent_protocol::OK);
		if(ret == extent_protocol::OK)cache_files[eid] = new cache_file(buf,a);
	}
	else if(cache_files[eid]->removed){
		extent_protocol::attr a;
		a.mtime = a.ctime = a.atime = current;
		a.size = buf.size();
		cache_files[eid] = new cache_file(buf,a,true,true);
	}
	else{
		cache_file* file = cache_files[eid];
		assert(file->meta_cached);
		file->buffer = buf;
		file->data_cached = true;
		file->data_dirty = true;
		file->meta_dirty = true;
		file->attr.mtime = file->attr.ctime = current;
		file->attr.size = buf.size();
	}
	printf("extent_client::put after put eid 0x%x, buf %s\n", (unsigned int)eid, cache_files[eid]->buffer.c_str());
	pthread_mutex_unlock(&cache_mutex);
	return ret;
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
	printf("extent_client::remove eid 0x%x\n", (unsigned int)eid);
	pthread_mutex_lock(&cache_mutex);
	extent_protocol::status ret = extent_protocol::OK;
	if(cache_files.find(eid) == cache_files.end()){
		int r;
		ret = cl->call(extent_protocol::remove, eid, r);
	}
	else{
		cache_files[eid]->removed = true;
	}
	pthread_mutex_unlock(&cache_mutex);
	return ret;
}

extent_protocol::status
extent_client::exist(extent_protocol::extentid_t eid, int& isexist)
{
	printf("extent_client::exist eid 0x%x\n", (unsigned int)eid);
	pthread_mutex_lock(&cache_mutex);
	extent_protocol::status ret = extent_protocol::OK;
	if(cache_files.find(eid) != cache_files.end() && !cache_files[eid]->removed){
		isexist = true;
	}
	else if(cache_files.find(eid) != cache_files.end() &&  cache_files[eid]->removed){
		isexist = false;
	}
	else{
		ret = cl->call(extent_protocol::exist, eid, isexist);
	}
	pthread_mutex_unlock(&cache_mutex);
	return ret;
}

extent_protocol::status
extent_client::flush(extent_protocol::extentid_t eid)
{
	printf("extent_client::flush eid 0x%x\n", (unsigned int)eid);
	pthread_mutex_lock(&cache_mutex);
	extent_protocol::status ret = extent_protocol::OK;
	if(cache_files.find(eid) == cache_files.end()){
		pthread_mutex_unlock(&cache_mutex);
		return ret;
	}
	cache_file* file = cache_files[eid];
	if(file->removed){
		int r;
		ret = cl->call(extent_protocol::remove, eid, r);
	}
	else{
		if(file->data_dirty){
			assert(file->data_cached);
			int r;
			ret = cl->call(extent_protocol::put, eid, file->buffer, r);
		}
		if(file->meta_dirty){
			assert(file->meta_cached);
			int r;
			ret = cl->call(extent_protocol::putattr, eid, file->attr, r);
		}
	}
	cache_files.erase(eid);
	pthread_mutex_unlock(&cache_mutex);
	return ret;
}

