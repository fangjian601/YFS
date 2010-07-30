TOPDIR	 = ../..
include $(TOPDIR)/Makeconf
BINDIR   = ../bin
HEADERS  = -I$(COMMON) -I$(RPC) -I../include/lock -I../include/filesystem -I../include/rsm
CXXFLAGS = -g -MD -Wall $(HEADERS)
LDFLAGS  = -L$(LIBDIR)  $(LIBDIR)/librpc.a $(LIBDIR)/librsm.a
LDLIBS	 = -lpthread
LDLIBS 	+= $(shell test -f `gcc -print-file-name=librt.so` && echo -lrt)
LDLIBS 	+= $(shell test -f `gcc -print-file-name=libdl.so` && echo -ldl)
OBJS	 = $(OBJDIR)/extent_server.o $(OBJDIR)/extent_smain.o \
		   $(OBJDIR)/master.o $(OBJDIR)/master_main.o \
		   $(OBJDIR)/chunkserver.o $(OBJDIR)/chunkserver_main.o
BIN		 = $(BINDIR)/extent_server $(BINDIR)/yfs_master $(BINDIR)/yfs_chunkserver

EXTENT_OBJS = $(OBJDIR)/extent_server.o $(OBJDIR)/extent_smain.o
MASTER_OBJS = $(OBJDIR)/master.o $(OBJDIR)/chunkserver.o $(OBJDIR)/master_main.o
CHUNK_OBJS  = $(OBJDIR)/master.o $(OBJDIR)/chunkserver.o $(OBJDIR)/chunkserver_main.o

all : $(BIN)

$(BINDIR)/extent_server : $(EXTENT_OBJS)
	$(CXX) -o $(BINDIR)/extent_server $(EXTENT_OBJS) $(LDFLAGS) $(LDLIBS)

$(BINDIR)/yfs_master : $(MASTER_OBJS)
	$(CXX) -o $(BINDIR)/yfs_master $(MASTER_OBJS) $(LDFLAGS) $(LDLIBS)

$(BINDIR)/yfs_chunkserver : $(CHUNK_OBJS)
	$(CXX) -o $(BINDIR)/yfs_chunkserver $(CHUNK_OBJS) $(LDFLAGS) $(LDLIBS)	

$(OBJDIR)/%.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean :
	rm $(OBJS) $(BIN)