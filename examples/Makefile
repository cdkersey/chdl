CXXFLAGS += -I../ -g -std=c++11
LDLIBS += -L../ -lchdl

GRAPHS = example0.ps example1.ps example2.ps example3.ps example4.ps example5.ps

EXAMPLE_OUTPUT = example0.vcd example1.vcd example2.vcd example3.vcd \
                 example4.vcd example5.vcd example6.vcd example7.vcd
EXAMPLES = example0 example1 example2 example3 example4 example5 example6 \
           example7

all: $(EXAMPLES) $(EXAMPLE_OUTPUT) $(GRAPHS)

%.ps : %.dot
	dot -Tps $< -o $@

%.vcd %.dot : %
	LD_LIBRARY_PATH=../ ./$<

clean:
	rm -f $(EXAMPLES) $(EXAMPLE_OUTPUT) *.nand *.vcd *.ps *.dot *~ *\#
