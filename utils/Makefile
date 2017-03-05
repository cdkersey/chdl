CXXFLAGS += -I../ -g -std=c++11
LDLIBS += -L../ -lchdl -pthread
UTILS ?= chdl-netl

all: $(UTILS)

% : %.cpp

clean:
	rm -f $(EXAMPLES) $(EXAMPLE_OUTPUT) *.nand *.vcd *.ps *.dot text.hex \
              *.hier *.netl *~ *\#
