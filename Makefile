PREFIX ?= /usr/local
CXXFLAGS += -fPIC -std=c++11 -O2 #-g
LDLIBS += -pthread

OBJS = gates.o nodeimpl.o tickable.o gatesimpl.o regimpl.o tap.o sim.o lit.o \
       memory.o opt.o netlist.o input.o analysis.o vis.o hierarchy.o \
       submodule.o latch.o techmap.o order.o tristate.o trisimpl.o reset.o \
       assert.o cdomain.o chdl_present.o execbuf.o

all : libchdl.so

libchdl.so : $(OBJS)
	g++ -shared $(LDFLAGS) -o $@ $^ $(LDLIBS)

install: libchdl.so
	cp libchdl.so $(PREFIX)/lib
	if [ ! -e $(PREFIX)/include/chdl ]; then mkdir $(PREFIX)/include/chdl; fi
	cp *.h $(PREFIX)/include/chdl

uninstall:
	rm -rf $(PREFIX)/lib/libchdl.so $(PREFIX)/include/chdl

assert.o: assert.cpp assert.h nodeimpl.h tickable.h node.h bvec.h bus.h tap.h \
          sim.h reset.h cdomain.h execbuf.h

reset.o: reset.cpp reset.h

gates.o: gates.cpp node.h gates.h nodeimpl.h gatesimpl.h sim.h cdomain.h \
         hierarchy.h cdomain.h tickable.h tap.h execbuf.h

gatesimpl.o: gatesimpl.cpp gatesimpl.h sim.h nodeimpl.h node.h hierarchy.h \
             cdomain.h tickable.h tap.h cdomain.h execbuf.h

lit.o: lit.cpp lit.h litimpl.h nodeimpl.h node.h hierarchy.h cdomain.h execbuf.h

memory.o: memory.cpp memory.h node.h bvec.h bvec-basic.h lit.h gates.h \
          hierarchy.h analysis.h reset.h cdomain.h

nodeimpl.o: nodeimpl.cpp nodeimpl.h node.h litimpl.h lit.h reset.h cdomain.h \
            execbuf.h

opt.o: opt.cpp opt.h nodeimpl.h gatesimpl.h sim.h litimpl.h lit.h node.h \
       gates.h memory.h hierarchy.h submodule.h regimpl.h input.h cdomain.h \
       tickable.h tap.h cdomain.h execbuf.h

vis.o: vis.cpp vis.h nodeimpl.h gatesimpl.h litimpl.h lit.h node.h gates.h \
       memory.h hierarchy.h cdomain.h execbuf.h

regimpl.o: regimpl.cpp reg.h regimpl.h nodeimpl.h tickable.h node.h gates.h \
           hierarchy.h cdomain.h execbuf.h

tickable.o: tickable.cpp tickable.h reset.h cdomain.h

sim.o: sim.cpp sim.h tickable.h tap.h reset.h cdomain.h

tap.o: tap.cpp tap.h nodeimpl.h node.h bvec.h gates.h hierarchy.h reset.h \
       cdomain.h execbuf.h

netlist.o: netlist.cpp netlist.h node.h nodeimpl.h tap.h input.h hierarchy.h \
           reg.h cdomain.h execbuf.h

input.o: input.cpp input.h node.h nodeimpl.h bvec.h gates.h hierarchy.h reset.h\
         cdomain.h execbuf.h

analysis.o: analysis.cpp opt.h tap.h gates.h nodeimpl.h gatesimpl.h litimpl.h \
            netlist.h lit.h node.h memory.h hierarchy.h regimpl.h cdomain.h \
            execbuf.h

hierarchy.o: hierarchy.cpp hierarchy.h reset.h cdomain.h

submodule.o: submodule.cpp submodule.h bvec.h node.h cdomain.h

latch.o: latch.cpp latch.h bvec.h bvec-basic.h gates.h reg.h lit.h node.h \
         hierarchy.h cdomain.h

techmap.o: techmap.cpp techmap.h gatesimpl.h gates.h regimpl.h reg.h node.h \
           nodeimpl.h tap.h trisimpl.h litimpl.h memory.h cdomain.h execbuf.h

tristate.o: tristate.cpp node.h tristate.h trisimpl.h hierarchy.h cdomain.h \
            execbuf.h

trisimpl.o: trisimpl.cpp tristate.h trisimpl.h node.h nodeimpl.h cdomain.h \
            execbuf.h

order.o: order.cpp adder.h analysis.h bvec-basic.h bvec-basic-op.h bvec.h \
         chdl.h divider.h enc.h gateops.h gates.h gatesimpl.h hierarchy.h \
         input.h latch.h lit.h litimpl.h llmem.h memory.h netlist.h node.h \
         nodeimpl.h opt.h reg.h regimpl.h shifter.h sim.h tap.h techmap.h \
         tickable.h vis.h cdomain.h execbuf.h

cdomain.o: cdomain.cpp cdomain.h tickable.h

chdl_present.o: chdl_present.cpp

execbuf.o: execbuf.cpp execbuf.h

clean:
	rm -f libchdl.so $(OBJS) *~ *\#
