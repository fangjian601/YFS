TOPDIR	 = ..
include $(TOPDIR)/Makeconf
BINDIR   = ../bin
HEADERS  = -I$(COMMON) -I$(RPC) -Iinclude
CXXFLAGS = -g -MD -Wall $(HEADERS)
LDFLAGS  = -L$(TOPDIR)/lib $(LIBDIR)/librpc.a
LDLIBS	 = -lpthread 
LDLIBS 	+= $(shell test -f `gcc -print-file-name=librt.so` && echo -lrt)
LDLIBS 	+= $(shell test -f `gcc -print-file-name=libdl.so` && echo -ldl)
OBJS	 = $(OBJDIR)/config.o $(OBJDIR)/handle.o $(OBJDIR)/log.o $(OBJDIR)/paxos.o \
		   $(OBJDIR)/rsm.o $(OBJDIR)/rsm_client.o $(OBJDIR)/rsmtest_client.o \
		   $(OBJDIR)/rsm_tester.o
LIB		 = $(LIBDIR)/librsm.a
BIN		 = $(TESTDIR)/bin/rsm_tester

all : $(LIB) $(BIN)

RSM_LIB_OBJS    = $(OBJDIR)/config.o $(OBJDIR)/handle.o $(OBJDIR)/log.o $(OBJDIR)/paxos.o \
		   		  $(OBJDIR)/rsm.o $(OBJDIR)/rsm_client.o
RSM_TESTER_OBJS = $(OBJDIR)/rsm_tester.o $(OBJDIR)/rsmtest_client.o $(OBJDIR)/rsm_client.o

$(LIB) : $(RSM_LIB_OBJS)
	$(RM) -f $@
	$(AR) cq $@ $^
	$(RANLIB) $(LIB)

$(TESTDIR)/bin/rsm_tester : $(RSM_TESTER_OBJS)
	$(CXX) -o $(TESTDIR)/bin/rsm_tester $(RSM_TESTER_OBJS) $(LDFLAGS) $(LDLIBS)	

$(OBJDIR)/%.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean :
	$(RM) $(OBJS) $(LIB) $(BIN)