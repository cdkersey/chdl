CXXFLAGS = -fPIC -O3 -std=c++11 #-g

OBJS = gates.o nodeimpl.o tickable.o gatesimpl.o regimpl.o tap.o sim.o lit.o \
       memory.o opt.o netlist.o

libchdl.so : $(OBJS)
	g++ -shared $(LDFLAGS) -o $@ $^ $(LDLIBS)

gates.o: gates.cpp node.h gates.h nodeimpl.h gatesimpl.h
gatesimpl.o: gatesimpl.cpp gatesimpl.h nodeimpl.h node.h
lit.o: lit.cpp lit.h litimpl.h nodeimpl.h node.h
memory.o: memory.cpp memory.h node.h bvec.h bvec-basic.h lit.h
nodeimpl.o: nodeimpl.cpp nodeimpl.h node.h litimpl.h
opt.o: opt.cpp opt.h nodeimpl.h gatesimpl.h litimpl.h lit.h node.h
regimpl.o: regimpl.cpp reg.h regimpl.h nodeimpl.h tickable.h node.h
sim.o: sim.cpp sim.h tickable.h tap.h
tap.o: tap.cpp tap.h nodeimpl.h node.h bvec.h
netlist.o: netlist.cpp netlist.h node.h nodeimpl.h

clean:
	rm -f libchdl.so $(OBJS) *~ *\#
