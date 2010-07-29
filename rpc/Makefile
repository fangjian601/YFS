TOPDIR	 = ..
include $(TOPDIR)/Makeconf
HEADERS  = -I$(RPC) -I$(COMMON)
CXXFLAGS = -g -MD -Wall $(HEADERS)
LDFLAGS  = -L$(TOPDIR)/lib
LDLIBS	 = -lpthread
LDLIBS 	+= $(shell test -f `gcc -print-file-name=librt.so` && echo -lrt)
LDLIBS 	+= $(shell test -f `gcc -print-file-name=libdl.so` && echo -ldl)
OBJS	 = $(OBJDIR)/connection.o $(OBJDIR)/jsl_log.o $(OBJDIR)/pollmgr.o \
		   $(OBJDIR)/rpc.o $(OBJDIR)/thr_pool.o $(OBJDIR)/gettime.o $(OBJDIR)/rpctest.o
LIB		 = $(LIBDIR)/librpc.a
BIN		 = $(TESTDIR)/bin/rpctest

all : $(LIB) $(BIN)

$(LIB) : $(OBJS)
	$(RM) -f $@
	$(AR) cq $@ $^
	$(RANLIB) $(LIB)

$(BIN) : $(OBJS)
	$(CXX) -o ${BIN} $(OBJS) $(LDFLAGS) $(LDLIBS)

$(OBJDIR)/%.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean :
	$(RM) $(OBJS) $(LIB) $(BIN)