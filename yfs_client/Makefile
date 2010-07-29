TOPDIR	 = ..
include $(TOPDIR)/Makeconf
BINDIR    = bin
HEADERS  = -I$(COMMON) -I$(RPC) -Iinclude -I/usr/include/fuse
FUSEFLAGS = -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=25 
CXXFLAGS = -g -MD -Wall $(HEADERS) $(FUSEFLAGS)
LDFLAGS  = -L$(LIBDIR) $(LIBDIR)/librpc.a
LDLIBS	 = -lpthread -lfuse
LDLIBS 	+= $(shell test -f `gcc -print-file-name=librt.so` && echo -lrt)
LDLIBS 	+= $(shell test -f `gcc -print-file-name=libdl.so` && echo -ldl)
OBJS	 = $(OBJDIR)/extent_client.o $(OBJDIR)/fuse.o $(OBJDIR)/lock_client_cache.o \
		   $(OBJDIR)/rsm_client.o $(OBJDIR)/yfs_client.o $(OBJDIR)/lock_client.o \
		   $(OBJDIR)/lock_tester.o $(OBJDIR)/rsmtest_client.o $(OBJDIR)/rsm_tester.o		   
BIN		 = $(BINDIR)/yfs_client $(TESTDIR)/bin/lock_tester $(TESTDIR)/bin/rsm_tester

YFS_CLIENT_OBJS = $(OBJDIR)/extent_client.o $(OBJDIR)/fuse.o \
				  $(OBJDIR)/lock_client_cache.o \
		   		  $(OBJDIR)/rsm_client.o $(OBJDIR)/yfs_client.o \
		   		  $(OBJDIR)/lock_client.o
LOCK_TESTER_OBJS = $(OBJDIR)/extent_client.o $(OBJDIR)/lock_client_cache.o \
				   $(OBJDIR)/lock_tester.o $(OBJDIR)/lock_client.o $(OBJDIR)/rsm_client.o
RSM_TESTER_OBJS = $(OBJDIR)/rsm_tester.o $(OBJDIR)/rsmtest_client.o $(OBJDIR)/rsm_client.o

all : $(BIN)

$(BINDIR)/yfs_client : $(YFS_CLIENT_OBJS)
	$(CXX) -o $(BINDIR)/yfs_client $(YFS_CLIENT_OBJS) $(LDFLAGS) $(LDLIBS)

$(TESTDIR)/bin/lock_tester : $(LOCK_TESTER_OBJS)
	$(CXX) -o $(TESTDIR)/bin/lock_tester $(LOCK_TESTER_OBJS) $(LDFLAGS) $(LDLIBS)

$(TESTDIR)/bin/rsm_tester : $(RSM_TESTER_OBJS)
	$(CXX) -o $(TESTDIR)/bin/rsm_tester $(RSM_TESTER_OBJS) $(LDFLAGS) $(LDLIBS)		

$(OBJDIR)/%.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean :
	rm $(OBJS) $(BIN)