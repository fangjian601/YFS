TOPDIR	 = ..
include $(TOPDIR)/Makeconf
BINDIR   = ../bin
HEADERS  = -I$(COMMON) -I$(RPC) -I$(RSM) -Iinclude
CXXFLAGS = -g -MD -Wall $(HEADERS)
LDFLAGS  = -L$(LIBDIR) $(LIBDIR)/librpc.a $(LIBDIR)/librsm.a
LDLIBS	 = -lpthread
LDLIBS 	+= $(shell test -f `gcc -print-file-name=librt.so` && echo -lrt)
LDLIBS 	+= $(shell test -f `gcc -print-file-name=libdl.so` && echo -ldl)
OBJS	 = $(OBJDIR)/lock_server_cache.o $(OBJDIR)/lock_client_cache.o \
		   $(OBJDIR)/lock_client.o $(OBJDIR)/lock_tester.o
LIB		 = $(LIBDIR)/liblock.a		   
BIN		 = $(TESTDIR)/bin/lock_tester

LOCK_LIB_OBJS = $(OBJDIR)/lock_server_cache.o $(OBJDIR)/lock_client_cache.o \
		   		$(OBJDIR)/lock_client.o 

all : $(LIB) $(BIN)

$(LIB) : $(LOCK_LIB_OBJS)
	$(RM) -f $@
	$(AR) cq $@ $^
	$(RANLIB) $(LIB)

$(BIN) : $(OBJS)
	$(CXX) -o ${BIN} $(OBJS) $(LDFLAGS) $(LDLIBS)

$(OBJDIR)/%.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean :
	rm $(OBJS) $(BIN) $(LIB)