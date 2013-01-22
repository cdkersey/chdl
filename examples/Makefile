CXXFLAGS = -I../ -O3 -g -std=c++11
LDLIBS = -L../ -lchdl

EXAMPLE_OUTPUT = example0.vcd example1.vcd example2.vcd example3.vcd \
                 example4.vcd example5.vcd example6.vcd example7.vcd
EXAMPLES = example0 example1 example2 example3 example4 example5 example6 \
           example7

all: $(EXAMPLES) $(EXAMPLE_OUTPUT)

%.vcd : %
	LD_LIBRARY_PATH=../ ./$< > $@

clean:
	rm -f $(EXAMPLES) $(EXAMPLE_OUTPUT) *.nand *~ *\#
