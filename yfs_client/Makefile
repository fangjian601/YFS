TOPDIR	 = ..
include $(TOPDIR)/Makeconf
BINDIR    = bin
HEADERS  = -I$(COMMON) -I$(RPC) -I$(RSM) -I$(LOCK) -Iinclude -I/usr/include/fuse
FUSEFLAGS = -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=25 
CXXFLAGS = -g -MD -Wall $(HEADERS) $(FUSEFLAGS)
LDFLAGS  = -L$(LIBDIR) $(LIBDIR)/librsm.a $(LIBDIR)/liblock.a $(LIBDIR)/librpc.a
LDLIBS	 = -lpthread -lfuse -lrpc -lrsm -llock
LDLIBS 	+= $(shell test -f `gcc -print-file-name=librt.so` && echo -lrt)
LDLIBS 	+= $(shell test -f `gcc -print-file-name=libdl.so` && echo -ldl)
OBJS	 = $(OBJDIR)/extent_client.o $(OBJDIR)/fuse.o $(OBJDIR)/yfs_client.o	   
BIN		 = $(BINDIR)/yfs_client

all : $(BIN)

$(BIN) : $(OBJS)
	$(CXX) -o $(BIN) $(OBJS) $(LDFLAGS) $(LDLIBS)

$(OBJDIR)/%.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean :
	rm $(OBJS) $(BIN)