PREFIX ?= /usr/local
CXXFLAGS += -fPIC -g -std=c++11 #-g

OBJS = gates.o nodeimpl.o tickable.o gatesimpl.o regimpl.o tap.o sim.o lit.o \
       memory.o opt.o netlist.o input.o analysis.o vis.o

UTILS = util/nand2v

all : libchdl.so $(UTILS)

util/% : 
	cd util; $(MAKE) $(MFLAGS) $(@F); cd ..

libchdl.so : $(OBJS)
	g++ -shared $(LDFLAGS) -o $@ $^ $(LDLIBS)

install: libchdl.so
	cp libchdl.so $(PREFIX)/lib
	cp $(UTILS) /usr/local/bin
	if [ ! -e $(PREFIX)/include/chdl ]; then mkdir $(PREFIX)/include/chdl; fi
	cp *.h $(PREFIX)/include/chdl

uninstall:
	rm -rf $(PREFIX)/lib/libchdl.so $(PREFIX)/include/chdl
	for x in $(UTILS); do\
	  rm -f $(PREFIX)/bin/`echo $$x | sed 's/.*\///'`;\
	done

gates.o: gates.cpp node.h gates.h nodeimpl.h gatesimpl.h sim.h
gatesimpl.o: gatesimpl.cpp gatesimpl.h sim.h nodeimpl.h node.h
lit.o: lit.cpp lit.h litimpl.h nodeimpl.h node.h
memory.o: memory.cpp memory.h node.h bvec.h bvec-basic.h lit.h gates.h
nodeimpl.o: nodeimpl.cpp nodeimpl.h node.h litimpl.h lit.h
opt.o: opt.cpp opt.h nodeimpl.h gatesimpl.h sim.h litimpl.h lit.h node.h \
       gates.h memory.h
vis.o: vis.cpp vis.h nodeimpl.h gatesimpl.h litimpl.h lit.h node.h gates.h \
       memory.h
regimpl.o: regimpl.cpp reg.h regimpl.h nodeimpl.h tickable.h node.h gates.h
sim.o: sim.cpp sim.h tickable.h tap.h
tap.o: tap.cpp tap.h nodeimpl.h node.h bvec.h gates.h
netlist.o: netlist.cpp netlist.h node.h nodeimpl.h tap.h input.h
input.o: input.cpp input.h node.h nodeimpl.h bvec.h gates.h
analysis.o: analysis.cpp opt.h tap.h gates.h nodeimpl.h gatesimpl.h litimpl.h \
            netlist.h lit.h node.h memory.h

clean:
	rm -f libchdl.so $(OBJS) *~ *\#
	cd util; make clean
