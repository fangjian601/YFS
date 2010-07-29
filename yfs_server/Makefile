SUBDIRS	  = rsm lock master 

define make_subdir
 @for subdir in $(SUBDIRS) ; do \
 ( cd $$subdir && make $1 && cd $(PWD)) \
 done;
endef

all:
	$(call make_subdir , all)
	
clean:
	$(call make_subdir , clean)
