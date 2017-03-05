PREFIX ?= /usr/local
CHDL_INCLUDE ?= $(PREFIX)/include
CXXFLAGS += -fPIC -std=c++11 -O2 -I$(CHDL_INCLUDE) #-g

OBJS = gates.o nodeimpl.o tickable.o gatesimpl.o regimpl.o tap.o sim.o lit.o \
       memory.o opt.o netlist.o input.o analysis.o vis.o hierarchy.o \
       submodule.o techmap.o tristate.o trisimpl.o reset.o assert.o \
       cdomain.o chdl_present.o ttable.o printable.o console.o parser.o

# These are not _all_ headers, just headers included in local .cpp files.
HEADERS = adder.h analysis.h assert.h bus.h bvec-basic.h bvec-basic-op.h \
          bvec.h cdomain.h chdl.h divider.h enc.h gateops.h gates.h \
          gatesimpl.h hierarchy.h input.h latch.h lit.h litimpl.h llmem.h \
          memory.h netlist.h node.h nodeimpl.h opt.h reg.h regimpl.h reset.h \
          shifter.h sim.h submodule.h tap.h techmap.h tickable.h trisimpl.h \
          tristate.h vis.h printable.h console.h parser.h

all : libchdl.so

libchdl.so : $(OBJS)
	$(CXX) -shared $(LDFLAGS) -o $@ $^

%.o : %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $<

install: libchdl.so
	if [ ! -e $(PREFIX)/lib ]; then mkdir -p $(PREFIX)/lib; fi
	cp libchdl.so $(PREFIX)/lib
	if [ ! -e $(PREFIX)/include/chdl ]; then mkdir -p $(PREFIX)/include/chdl; fi
	cp *.h $(PREFIX)/include/chdl

uninstall:
	rm -rf $(PREFIX)/lib/libchdl.so $(PREFIX)/include/chdl

clean:
	$(RM) libchdl.so $(OBJS) *~ *\#
