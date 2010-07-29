TOPDIR	 = ..
include $(TOPDIR)/Makeconf
BINDIR    = bin
HEADERS   = -I$(COMMON) -I$(RPC) -Iinclude
CXXFLAGS  = -g -MD -Wall $(HEADERS)
LDFLAGS   = -L$(LIBDIR)
LDLIBS	  = -lpthread -lfuse
LDLIBS 	 += $(shell test -f `gcc -print-file-name=librt.so` && echo -lrt)
LDLIBS 	 += $(shell test -f `gcc -print-file-name=libdl.so` && echo -ldl)
OBJS	  = $(OBJDIR)/test-lab-4-b.o $(OBJDIR)/test-lab-4-c.o
BIN		  = $(BINDIR)/test-lab-4-b $(BINDIR)/test-lab-4-c

all : $(BIN)

$(BINDIR)/test-lab-4-b : $(OBJDIR)/test-lab-4-b.o
	$(CXX) -o $(BINDIR)/test-lab-4-b $(OBJDIR)/test-lab-4-b.o $(LDFLAGS) $(LDLIBS)
	
$(BINDIR)/test-lab-4-c : $(OBJDIR)/test-lab-4-c.o
	$(CXX) -o $(BINDIR)/test-lab-4-c $(OBJDIR)/test-lab-4-c.o $(LDFLAGS) $(LDLIBS)

$(OBJDIR)/%.o : %.c
	$(CC) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@
		

clean :
	rm $(OBJS) $(BIN)