TOPDIR	 = ../..
include $(TOPDIR)/Makeconf
BINDIR   = ../bin
HEADERS  = -I$(COMMON) -I$(RPC) -I../include/lock -I../include/master -I../include/rsm
CXXFLAGS = -g -MD -Wall $(HEADERS)
LDFLAGS  = -L$(TOPDIR)/lib  $(LIBDIR)/librpc.a
LDLIBS	 = -lpthread
LDLIBS 	+= $(shell test -f `gcc -print-file-name=librt.so` && echo -lrt)
LDLIBS 	+= $(shell test -f `gcc -print-file-name=libdl.so` && echo -ldl)
OBJS	 = $(OBJDIR)/config.o $(OBJDIR)/handle.o $(OBJDIR)/log.o $(OBJDIR)/paxos.o \
		   $(OBJDIR)/rsm.o
LIB		 = $(LIBDIR)/librsm.a

all : $(LIB)

$(LIB) : $(OBJS)
	$(RM) -f $@
	$(AR) cq $@ $^
	$(RANLIB) $(LIB)

$(OBJDIR)/%.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean :
	$(RM) $(OBJS) $(LIB)