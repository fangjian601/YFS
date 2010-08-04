/*
 * master.cc
 *
 *  Created on: 2010-7-30
 *      Author: frank
 */

#include "master.h"
#include <stdio.h>

master::master(class rsm* _rsm) :
	rsm_server(_rsm) {

	pthread_mutex_init(&fileservers_mutex, NULL);
	pthread_mutex_init(&files_mutex, NULL);

	rsm_server->reg(master_protocol::add_fileserver, this,
			&master::add_fileserver);
	rsm_server->reg(master_protocol::remove_fileserver, this,
			&master::remove_fileserver);
	rsm_server->reg(master_protocol::heartbeat, this, &master::heartbeat);

	rsm_server->reg(master_client_protocol::access, this, &master::access);
	rsm_server->reg(master_client_protocol::create, this, &master::create);
	rsm_server->reg(master_client_protocol::flush, this, &master::flush);
	rsm_server->reg(master_client_protocol::getattr, this, &master::getattr);
	rsm_server->reg(master_client_protocol::link, this, &master::link);
	rsm_server->reg(master_client_protocol::mkdir, this, &master::mkdir);
	rsm_server->reg(master_client_protocol::open, this, &master::open);
	rsm_server->reg(master_client_protocol::read, this, &master::read);
	rsm_server->reg(master_client_protocol::readdir, this, &master::readdir);
	rsm_server->reg(master_client_protocol::readlink, this, &master::readlink);
	rsm_server->reg(master_client_protocol::rename, this, &master::rename);
	rsm_server->reg(master_client_protocol::rmdir, this, &master::rmdir);
	rsm_server->reg(master_client_protocol::setattr, this, &master::setattr);
	rsm_server->reg(master_client_protocol::symlink, this, &master::symlink);
	rsm_server->reg(master_client_protocol::unlink, this, &master::unlink);
	rsm_server->reg(master_client_protocol::write, this, &master::write);

	rsm_server->set_state_transfer(this);
}

master::~master() {
}

std::string master::marshal_state() {
	marshall m;
	pthread_mutex_lock(&files_mutex);
	m << files.size();
	std::map<file_protocol::file_t, file_protocol::metadata>::iterator
			files_iter = files.begin();
	for (; files_iter != files.end(); ++files_iter) {
		m << files_iter->first;
		m << files_iter->second;
	}
	pthread_mutex_unlock(&files_mutex);

	pthread_mutex_lock(&fileservers_mutex);
	m << replicates.size();
	std::map<file_protocol::file_t, std::set<std::string> >::iterator
			replicates_iter = replicates.begin();
	for(; replicates_iter != replicates.end(); ++replicates_iter){
		m << replicates_iter->first;
		m << replicates_iter->second;
	}

	m << fileservers.size();
	std::map<std::string, fileserver_info*>::iterator fileservers_iter = fileservers.begin();
	for(; fileservers_iter != fileservers.end(); ++fileservers_iter){
		m << fileservers_iter->first;
		m << fileservers_iter->second->files;
	}
	pthread_mutex_unlock(&fileservers_mutex);
	return m.str();
}

void master::unmarshal_state(std::string state) {
	unmarshall u(state);
	uint size;
	u >> size;
	pthread_mutex_lock(&files_mutex);
	for(uint i=0; i<size; i++){
		file_protocol::file_t file;
		u >> file;
		file_protocol::metadata meta;
		u >> meta;
		files[file] = meta;
	}
	pthread_mutex_unlock(&files_mutex);

	u >> size;
	pthread_mutex_lock(&fileservers_mutex);
	for(uint i=0; i<size; i++){
		file_protocol::file_t file;
		u >> file;
		std::set<std::string> reps;
		u >> reps;
		replicates[file] = reps;
	}

	u >> size;
	for(uint i=0; i<size; i++){
		std::string id;
		u >> id;
		std::vector<file_protocol::file_t> files;
		u >> files;
		fileserver_info* finfo = new fileserver_info(id, files, this);
		fileservers[id] = finfo;
	}
	pthread_mutex_unlock(&fileservers_mutex);
}

void master::delete_fileserver(std::string id) {
	pthread_mutex_lock(&fileservers_mutex);
	if (fileservers.find(id) == fileservers.end())
		return;
	delete_fileserver_wo(id);
	pthread_mutex_unlock(&fileservers_mutex);
}

void master::delete_fileserver_wo(std::string id) {
	std::vector<file_protocol::file_t> files = fileservers[id]->get_files();
	for (uint i = 0; i < files.size(); i++) {
		if (replicates[files[i]].find(id) != replicates[files[i]].end()) {
			replicates[files[i]].erase(id);
		}
	}
	fileservers.erase(id);
}

master_protocol::status master::add_fileserver(std::string id, std::vector<
		file_protocol::file_t> files, int&) {
	printf("master::add_fileserver: id %s\n", id.c_str());
	master_protocol::status ret = master_protocol::OK;
	pthread_mutex_lock(&fileservers_mutex);
	if (fileservers.find(id) != fileservers.end()) {
		ret = master_protocol::EXIST;
	} else {
		fileservers[id] = new fileserver_info(id, files, this);
		for (uint i = 0; i < files.size(); i++) {
			replicates[files[i]].insert(id);
		}
		ret = master_protocol::OK;
	}
	pthread_mutex_unlock(&fileservers_mutex);
	return ret;
}
master_protocol::status master::remove_fileserver(std::string id, int &) {
	printf("master:remove_fileserver: id %s\n", id.c_str());
	master_protocol::status ret = master_protocol::OK;
	pthread_mutex_lock(&fileservers_mutex);
	if (fileservers.find(id) == fileservers.end()) {
		ret = master_protocol::NOENT;
	} else {
		delete_fileserver_wo(id);
		ret = master_protocol::OK;
	}
	pthread_mutex_unlock(&fileservers_mutex);
	return ret;
}
master_protocol::status master::heartbeat(std::string id, int &) {
	printf("master::heartbeat: heartbeat from fileserver %s\n", id.c_str());
	master_protocol::status ret = master_protocol::OK;
	pthread_mutex_lock(&fileservers_mutex);
	if (fileservers.find(id) == fileservers.end()) {
		ret = master_protocol::ERR;
	} else {
		pthread_mutex_lock(&fileservers[id]->heartbeat_mutex);
		fileservers[id]->alive = true;
		pthread_cond_signal(&fileservers[id]->heartbeat_cond);
		pthread_mutex_unlock(&fileservers[id]->heartbeat_mutex);
		ret = master_protocol::OK;
	}
	pthread_mutex_unlock(&fileservers_mutex);
	return ret;
}

master_client_protocol::status master::read(file_protocol::file_t ino,
		std::vector<std::string>& reps) {
	master_client_protocol::status ret = master_client_protocol::OK;
	return ret;
}
master_client_protocol::status master::write(file_protocol::file_t ino,
		std::vector<std::string>& reps) {
	master_client_protocol::status ret = master_client_protocol::OK;
	return ret;
}
master_client_protocol::status master::flush(file_protocol::file_t ino,
		std::vector<std::string>& reps) {
	master_client_protocol::status ret = master_client_protocol::OK;
	return ret;
}
master_client_protocol::status master::open(file_protocol::file_t ino, int &) {
	master_client_protocol::status ret = master_client_protocol::OK;
	return ret;
}
master_client_protocol::status master::getattr(file_protocol::file_t ino,
		file_protocol::metadata& attr) {
	master_client_protocol::status ret = master_client_protocol::OK;
	return ret;
}
master_client_protocol::status master::setattr(file_protocol::file_t ino,
		file_protocol::metadata attr, int &) {
	master_client_protocol::status ret = master_client_protocol::OK;
	return ret;
}
master_client_protocol::status master::readlink(file_protocol::file_t ino,
		std::string& link) {
	master_client_protocol::status ret = master_client_protocol::OK;
	return ret;
}
master_client_protocol::status master::link(file_protocol::file_t parent,
		std::string name, file_protocol::metadata& attr) {
	master_client_protocol::status ret = master_client_protocol::OK;
	return ret;
}
master_client_protocol::status master::symlink(file_protocol::file_t parent,
		std::string name, file_protocol::metadata& attr) {
	master_client_protocol::status ret = master_client_protocol::OK;
	return ret;
}
master_client_protocol::status master::readdir(file_protocol::file_t ino,
		file_protocol::entries_t& entries) {
	master_client_protocol::status ret = master_client_protocol::OK;
	return ret;
}
master_client_protocol::status master::mkdir(file_protocol::file_t parent,
		std::string name, mode_t mode, file_protocol::metadata& attr) {
	master_client_protocol::status ret = master_client_protocol::OK;
	return ret;
}
master_client_protocol::status master::rmdir(file_protocol::file_t parent,
		std::string name, int &) {
	master_client_protocol::status ret = master_client_protocol::OK;
	return ret;
}
master_client_protocol::status master::rename(file_protocol::file_t parent,
		std::string name, file_protocol::file_t newparent, std::string newname,
		int &) {
	master_client_protocol::status ret = master_client_protocol::OK;
	return ret;
}
master_client_protocol::status master::create(file_protocol::file_t parent,
		std::string name, mode_t mode, std::vector<std::string>& reps) {
	master_client_protocol::status ret = master_client_protocol::OK;
	return ret;
}
master_client_protocol::status master::unlink(file_protocol::file_t parent,
		std::string name, std::vector<std::string>& reps) {
	master_client_protocol::status ret = master_client_protocol::OK;
	return ret;
}
master_client_protocol::status master::access(file_protocol::file_t ino,
		int mask, int &) {
	master_client_protocol::status ret = master_client_protocol::OK;
	return ret;
}
