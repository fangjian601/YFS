TOPDIR	 = ../..
include $(TOPDIR)/Makeconf
BINDIR   = ../bin
HEADERS  = -I$(COMMON) -I$(RPC) -I../include/lock -I../include/master -I../include/rsm
CXXFLAGS = -g -MD -Wall $(HEADERS)
LDFLAGS  = -L$(LIBDIR)  $(LIBDIR)/librpc.a
LDLIBS	 = -lpthread
LDLIBS 	+= $(shell test -f `gcc -print-file-name=librt.so` && echo -lrt)
LDLIBS 	+= $(shell test -f `gcc -print-file-name=libdl.so` && echo -ldl)
OBJS	 = $(OBJDIR)/extent_server.o $(OBJDIR)/extent_smain.o
BIN		 = $(BINDIR)/yfs_master

all : $(BIN)

$(BIN) : $(OBJS)
	$(CXX) -o ${BIN} $(OBJS) $(LDFLAGS) $(LDLIBS)

$(OBJDIR)/%.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean :
	rm $(OBJS) $(BIN)