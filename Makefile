SUBDIRS = rpc tests yfs_client yfs_server

define make_subdir
 @for subdir in $(SUBDIRS) ; do \
 ( cd $$subdir && make $1 && cd $(PWD)) \
 done;
endef

all:
	$(call make_subdir , all)
	
clean:
	$(call make_subdir , clean)