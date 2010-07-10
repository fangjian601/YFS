/*
 * receive request from fuse and call methods of yfs_client
 *
 * started life as low-level example in the fuse distribution.  we
 * have to use low-level interface in order to get i-numbers.  the
 * high-level interface only gives us complete paths.
 */

#include <fuse_lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <arpa/inet.h>
#include "yfs_client.h"

#include <set>

int myid;
yfs_client *yfs;

int id() { 
  return myid;
}

yfs_client::status
getattr(yfs_client::inum inum, struct stat &st)
{
	yfs_client::status ret;

	bzero(&st, sizeof(st));

	st.st_ino = inum;
	printf("getattr %016llx %d\n", inum, yfs->isfile(inum));
	if(yfs->isfile(inum)){
		extent_protocol::attr info;
		ret = yfs->getattr(inum, info);
		if(ret != yfs_client::OK)
		return ret;
		st.st_mode = S_IFREG | 0666;
		st.st_nlink = 1;
		st.st_atime = info.atime;
		st.st_mtime = info.mtime;
		st.st_ctime = info.ctime;
		st.st_size = info.size;
		//printf("   getattr -> %llu\n", info.size);
	} else {
		extent_protocol::attr info;
		ret = yfs->getattr(inum, info);
		if(ret != yfs_client::OK)
			return ret;
		st.st_mode = S_IFDIR | 0777;
		st.st_nlink = 2;
		st.st_atime = info.atime;
		st.st_mtime = info.mtime;
		st.st_ctime = info.ctime;
		//printf("   getattr -> %lu %lu %lu\n", info.atime, info.mtime, info.ctime);
	}
	return yfs_client::OK;
}


void
fuseserver_getattr(fuse_req_t req, fuse_ino_t ino,
          struct fuse_file_info *fi)
{
    struct stat st;
    yfs_client::inum inum = ino; // req->in.h.nodeid;
    yfs_client::status ret;
    yfs->acquire(inum);
    ret = getattr(inum, st);
    if(ret != yfs_client::OK){
    	yfs->release(inum);
		fuse_reply_err(req, ENOENT);
		return;
    }
    yfs->release(inum);
    fuse_reply_attr(req, &st, 0);
}

void
fuseserver_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr, int to_set, struct fuse_file_info *fi)
{
	printf("fuseserver_setattr 0x%x\n", to_set);

	struct stat st;

	extent_protocol::attr attribute;
	yfs_client::inum inumber = ino;
	std::string buf;

	time_t current_time;
	time(&current_time);

	yfs->acquire(inumber);

	if(yfs->getattr(inumber,attribute) != yfs_client::OK ||
	   getattr(inumber, st) != yfs_client::OK){
		yfs->release(inumber);
		fuse_reply_err(req,EIO);
	}

	else if(yfs->get(inumber,buf) != yfs_client::OK){
		yfs->release(inumber);
		fuse_reply_err(req,EIO);
	}

	else{
		if (FUSE_SET_ATTR_SIZE & to_set) {
			printf("fuseserver_setattr set size to %zu\n", (size_t)attr->st_size);
			// You fill this in
			if(attr->st_size == 0){
				buf.clear();
			}
			else if(attr->st_size <= attribute.size){
				buf = buf.substr(0,attr->st_size);
			}
			else{
				std::stringstream sstr;
				sstr << buf;
				for(int i=0; i<attr->st_size-attribute.size; i++){
					sstr<<"\0";
				}
				buf = sstr.str();
			}
			if(yfs->put(inumber,buf) != yfs_client::OK){
				yfs->release(inumber);
				fuse_reply_err(req,EIO);
			}
			else{
				attribute.size = attr->st_size;
				attribute.mtime = current_time;
				attribute.ctime = current_time;
				st.st_size = attr->st_size;
			}
		}
		if (FUSE_SET_ATTR_ATIME & to_set){
			printf("fuseserver_setattr set atime to 0x%x\n", (unsigned int)attr->st_atime);
			attribute.atime = attr->st_atime;
			st.st_atime = attr->st_atime;
		}
		if (FUSE_SET_ATTR_MTIME & to_set){
			printf("fuseserver_setattr set mtime to 0x%x\n", (unsigned int)attr->st_mtime);
			attribute.mtime = attr->st_mtime;
			st.st_mtime = attr->st_mtime;
		}
		if (FUSE_SET_ATTR_ATIME_NOW & to_set){
			printf("fuseserver_setattr set atime now to 0x%x\n", (unsigned int)current_time);
			attribute.atime = current_time;
			st.st_atime = current_time;
		}
		if (FUSE_SET_ATTR_MTIME_NOW & to_set){
			printf("fuseserver_setattr set mtime now to 0x%x\n", (unsigned int)current_time);
			attribute.mtime = current_time;
			st.st_mtime = current_time;
		}

		if (yfs->putattr(inumber, attribute) == yfs_client::OK){
			yfs->release(inumber);
			fuse_reply_attr(req,&st,0);
		}
		else{
			yfs->release(inumber);
			fuse_reply_err(req,EIO);
		}
	}


}

void
fuseserver_read(fuse_req_t req, fuse_ino_t ino, size_t size,
      off_t off, struct fuse_file_info *fi)
{
	printf("fuseserver_read: ino 0x%x, size %d, off %d\n", (unsigned int)ino, size, (int)off);
	// You fill this in
	yfs_client::inum inumber = ino;
	std::string buf;

	yfs->acquire(inumber);

	if(yfs->get(inumber,buf) == yfs_client::IOERR){
		yfs->release(inumber);
		fuse_reply_err(req,ENOENT);
	}
	else{
		if(off < 0 || size < 0){
			yfs->release(inumber);
			fuse_reply_err(req,EIO);
		}
		else if(off >= buf.size() ){
			yfs->release(inumber);
			fuse_reply_buf(req,NULL,0);
		}
		else if(off+size <= buf.size()){
			yfs->release(inumber);
			fuse_reply_buf(req,buf.substr(off,size).c_str(),size);
		}
		else{
			yfs->release(inumber);
			fuse_reply_buf(req,buf.substr(off,buf.size()-off).c_str(),buf.size()-off);
		}
	}

}

void
fuseserver_write(fuse_req_t req, fuse_ino_t ino,
  const char *buf, size_t size, off_t off,
  struct fuse_file_info *fi)
{
	// You fill this in

	printf("fuseserver_write: ino 0x%x, size %d, off %d, flags 0x%x\n",
			(unsigned int)ino, size, (int)off, fi->flags);

	yfs_client::inum inumber = ino;
	std::string data;

	yfs->acquire(inumber);

	if(yfs->get(inumber,data) == yfs_client::IOERR){
		yfs->release(inumber);
		fuse_reply_err(req,ENOENT);
	}
	else{
		std::stringstream sstr;
		if(off < 0 || off > data.size() || size < 0){
			yfs->release(inumber);
			fuse_reply_err(req,EIO);
		}
		else{
			if(off > 0)sstr << data.substr(0,off);

			sstr << std::string(buf,size);

			if(off+size < data.size()){
				sstr << data.substr(off+size,std::string::npos);
			}

			if(yfs->put(inumber,sstr.str()) == yfs_client::IOERR){
				yfs->release(inumber);
				fuse_reply_err(req,EIO);
			}
			else{
				yfs->release(inumber);
				fuse_reply_write(req,size);
			}
		}
	}
}

yfs_client::status
fuseserver_createhelper(fuse_ino_t parent, const char *name,
     mode_t mode, struct fuse_entry_param *e, bool isdir)
{
	printf("fuseserver_createhelper: create file %s\n",name);
	// You fill this in
	fuse_ino_t new_ino;
	yfs->acquire(parent);
	yfs_client::inum inumber = yfs->ilookup(parent,name);
	srand(getpid());
	if(isdir && inumber == 0){
		do{
			new_ino = rand();
			yfs->acquire(new_ino);
			if(yfs->exist(new_ino)){
				yfs->release(new_ino);
			}
			else break;
		}while(true);
	}
	else if(isdir && inumber != 0){
		yfs->release(parent);
		return yfs_client::EXIST;
	}
	else if(!isdir && inumber == 0){
		do{
			new_ino = rand() | 0x80000000;
			yfs->acquire(new_ino);
			if(yfs->exist(new_ino)){
				yfs->release(new_ino);
			}
			else break;
		}while(true);
	}
	else{
		yfs->release(parent);
		return yfs_client::EXIST;
	}

	std::string buf;
	std::string parent_buf;
	if(yfs->put(new_ino,buf) == yfs_client::IOERR){
		yfs->release(new_ino);
		yfs->release(parent);
		return yfs_client::IOERR;
	}

	std::stringstream sstr;
	if(yfs->get(parent,parent_buf) == yfs_client::IOERR){
		yfs->release(new_ino);
		yfs->release(parent);
		return yfs_client::IOERR;
	}

	sstr << parent_buf;
	sstr << new_ino << " " << name << "\n";

	if(yfs->put(parent,sstr.str()) == yfs_client::IOERR){
		yfs->release(new_ino);
		yfs->release(parent);
		return yfs_client::IOERR;
	}

	struct stat st;
	getattr(new_ino,st);
	e->ino = new_ino;
	e->generation = 1;
	e->attr = st;
	e->attr_timeout = 0.0;
	e->entry_timeout = 0.0;

	yfs->release(new_ino);
	yfs->release(parent);

	return yfs_client::OK;
}

void
fuseserver_create(fuse_req_t req, fuse_ino_t parent, const char *name,
   mode_t mode, struct fuse_file_info *fi)
{
	printf("fuseserver_create: parent 0x%x, name %s\n", (unsigned int)parent, name);

	struct fuse_entry_param e;
	yfs_client::status ret = fuseserver_createhelper( parent, name, mode, &e, false);
	if( ret == yfs_client::OK ) {
		fuse_reply_create(req, &e, fi);
	}
	else if( ret == yfs_client::EXIST ){
		fuse_reply_err(req,EEXIST);
	}
	else {
		fuse_reply_err(req, ENOENT);
	}
}

void fuseserver_mknod( fuse_req_t req, fuse_ino_t parent, 
    const char *name, mode_t mode, dev_t rdev ) {
  struct fuse_entry_param e;
  if( fuseserver_createhelper( parent, name, mode, &e, false) == yfs_client::OK ) {
    fuse_reply_entry(req, &e);
  } else {
    fuse_reply_err(req, ENOENT);
  }
}

void
fuseserver_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
	struct fuse_entry_param e;
	bool found = false;

	e.attr_timeout = 0.0;
	e.entry_timeout = 0.0;

	// You fill this in:
	// Look up the file named `name' in the directory referred to by
	// `parent' in YFS. If the file was found, initialize e.ino and
	// e.attr appropriately.

	printf("fuseserver_lookup: parent 0x%x, name %s\n", (unsigned int)parent, name);

	yfs->acquire(parent);
	yfs_client::inum inumber = yfs->ilookup(parent,name);
	yfs->release(parent);

	found = (inumber != 0)?true:false;

	if (found){
		struct stat st;
		yfs->acquire(inumber);
		getattr(inumber,st);
		yfs->release(inumber);
		e.ino = inumber;
		e.attr = st;
		fuse_reply_entry(req, &e);
	}
	else
		fuse_reply_err(req, ENOENT);
}


struct dirbuf {
    char *p;
    size_t size;
};

void dirbuf_add(struct dirbuf *b, const char *name, fuse_ino_t ino)
{
    struct stat stbuf;
    size_t oldsize = b->size;
    b->size += fuse_dirent_size(strlen(name));
    b->p = (char *) realloc(b->p, b->size);
    memset(&stbuf, 0, sizeof(stbuf));
    stbuf.st_ino = ino;
    fuse_add_dirent(b->p + oldsize, name, &stbuf, b->size);
}

#define min(x, y) ((x) < (y) ? (x) : (y))

int reply_buf_limited(fuse_req_t req, const char *buf, size_t bufsize,
          off_t off, size_t maxsize)
{
  if (off < bufsize)
    return fuse_reply_buf(req, buf + off, min(bufsize - off, maxsize));
  else
    return fuse_reply_buf(req, NULL, 0);
}

void
fuseserver_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
          off_t off, struct fuse_file_info *fi)
{
	yfs_client::inum inum = ino; // req->in.h.nodeid;
	struct dirbuf b;
	yfs_client::dirent e;

	printf("fuseserver_readdir\n");

	if(!yfs->isdir(inum)){
		fuse_reply_err(req, ENOTDIR);
		return;
	}

	memset(&b, 0, sizeof(b));


	// fill in the b data structure using dirbuf_add
	std::string parent_buf;
	yfs->acquire(ino);
	if(yfs->get(ino,parent_buf) == yfs_client::IOERR){
		yfs->release(ino);
		fuse_reply_err(req,ENOENT);
	}
	else{
		yfs->release(ino);
		std::vector<std::string> entries = yfs_client::split(parent_buf,"\n",true,false);

		for(unsigned int i = 0; i < entries.size(); i++){
			std::string entry = entries[i];
			std::vector<std::string> info = yfs_client::split(entry," ", true,false);
			if(info.size() != 2){
				fuse_reply_err(req,EIO);
			}
			yfs_client::inum temp_inum = yfs_client::n2i(info[0]);
			std::string temp_name = info[1];
			dirbuf_add(&b,temp_name.c_str(),temp_inum);
		}

		reply_buf_limited(req, b.p, b.size, off, size);
		free(b.p);
	}
 }


void
fuseserver_open(fuse_req_t req, fuse_ino_t ino,
     struct fuse_file_info *fi)
{
	// You fill this in
	yfs->acquire(ino);
	if(yfs->exist(ino)){
		yfs->release(ino);
		fuse_reply_open(req, fi);
	}
	else{
		yfs->release(ino);
		fuse_reply_err(req,ENOENT);
	}
}

void
fuseserver_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
     mode_t mode)
{
	struct fuse_entry_param e;
	if(fuseserver_createhelper( parent, name, mode, &e, true) == yfs_client::OK ){
		fuse_reply_entry(req, &e);
	}
	else{
		fuse_reply_err(req, EIO);
	}
}

void
fuseserver_unlink(fuse_req_t req, fuse_ino_t parent, const char *name)
{

  // You fill this in
  // Success:	fuse_reply_err(req, 0);
  // Not found:	fuse_reply_err(req, ENOENT);
  std::string parent_buf;
  yfs->acquire(parent);
  if(yfs->get(parent,parent_buf) != yfs_client::OK){
	  yfs->release(parent);
	  fuse_reply_err(req,EIO);
  }
  else{
	  yfs_client::inum inumber = yfs->ilookup(parent,name);
	  yfs->acquire(inumber);
	  if(inumber == 0){
		  yfs->release(inumber);
		  yfs->release(parent);
		  fuse_reply_err(req,ENOENT);
	  }
	  else if(yfs->remove(inumber) != yfs_client::OK){
		  yfs->release(inumber);
		  yfs->release(parent);
		  fuse_reply_err(req,EIO);
	  }
	  else{
		  	yfs->release(inumber);
			std::vector<std::string> entries = yfs_client::split(parent_buf,"\n",true,false);
			std::stringstream sstr;
			for(unsigned int i = 0; i < entries.size(); i++){
				std::string entry = entries[i];
				std::vector<std::string> info = yfs_client::split(entry," ", true,false);
				if(info.size() != 2){
					fuse_reply_err(req,EIO);
				}
				yfs_client::inum temp_inum = yfs_client::n2i(info[0]);
				if(temp_inum != inumber){
					sstr << entry << "\n";
				}
			}
			if(yfs->put(parent,sstr.str())!= yfs_client::OK){
				yfs->release(parent);
				fuse_reply_err(req,EIO);
			}
			else{
				yfs->release(parent);
				fuse_reply_err(req,0);
			}
	  }
  }
}

void
fuseserver_statfs(fuse_req_t req)
{
  struct statvfs buf;

  printf("statfs\n");

  memset(&buf, 0, sizeof(buf));

  buf.f_namemax = 255;
  buf.f_bsize = 512;

  fuse_reply_statfs(req, &buf);
}

struct fuse_lowlevel_ops fuseserver_oper;

int
main(int argc, char *argv[])
{
  char *mountpoint = 0;
  int err = -1;
  int fd;

  setvbuf(stdout, NULL, _IONBF, 0);

  if(argc != 4){
    fprintf(stderr, "Usage: yfs_client <mountpoint> <port-extent-server> <port-lock-server>\n");
    exit(1);
  }
  mountpoint = argv[1];

  srandom(getpid());

  myid = random();

  yfs = new yfs_client(argv[2], argv[3]);

  fuseserver_oper.getattr    = fuseserver_getattr;
  fuseserver_oper.statfs     = fuseserver_statfs;
  fuseserver_oper.readdir    = fuseserver_readdir;
  fuseserver_oper.lookup     = fuseserver_lookup;
  fuseserver_oper.create     = fuseserver_create;
  fuseserver_oper.mknod      = fuseserver_mknod;
  fuseserver_oper.open       = fuseserver_open;
  fuseserver_oper.read       = fuseserver_read;
  fuseserver_oper.write      = fuseserver_write;
  fuseserver_oper.setattr    = fuseserver_setattr;
  fuseserver_oper.unlink     = fuseserver_unlink;
  fuseserver_oper.mkdir      = fuseserver_mkdir;

  const char *fuse_argv[20];
  int fuse_argc = 0;
  fuse_argv[fuse_argc++] = argv[0];
#ifdef __APPLE__
  fuse_argv[fuse_argc++] = "-o";
  fuse_argv[fuse_argc++] = "nolocalcaches"; // no dir entry caching
  fuse_argv[fuse_argc++] = "-o";
  fuse_argv[fuse_argc++] = "daemon_timeout=86400";
#endif

  // everyone can play, why not?
  //fuse_argv[fuse_argc++] = "-o";
  //fuse_argv[fuse_argc++] = "allow_other";

  fuse_argv[fuse_argc++] = mountpoint;
  fuse_argv[fuse_argc++] = "-d";

  fuse_args args = FUSE_ARGS_INIT( fuse_argc, (char **) fuse_argv );
  int foreground;
  int res = fuse_parse_cmdline( &args, &mountpoint, 0 /*multithreaded*/, 
        &foreground );
  if( res == -1 ) {
    fprintf(stderr, "fuse_parse_cmdline failed\n");
    return 0;
  }
  
  args.allocated = 0;

  fd = fuse_mount(mountpoint, &args);
  if(fd == -1){
    fprintf(stderr, "fuse_mount failed\n");
    exit(1);
  }

  struct fuse_session *se;

  se = fuse_lowlevel_new(&args, &fuseserver_oper, sizeof(fuseserver_oper),
       NULL);
  if(se == 0){
    fprintf(stderr, "fuse_lowlevel_new failed\n");
    exit(1);
  }

  struct fuse_chan *ch = fuse_kern_chan_new(fd);
  if (ch == NULL) {
    fprintf(stderr, "fuse_kern_chan_new failed\n");
    exit(1);
  }

  fuse_session_add_chan(se, ch);
  // err = fuse_session_loop_mt(se);   // FK: wheelfs does this; why?
  err = fuse_session_loop(se);
    
  fuse_session_destroy(se);
  close(fd);
  fuse_unmount(mountpoint);

  return err ? 1 : 0;
}
