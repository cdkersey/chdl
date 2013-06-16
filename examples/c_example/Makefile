CXXFLAGS += -g -std=c++11
LDLIBS += -lchdl

all: out.dump

out.dump : example-c
	./example-c > out.dump

example-c : example-c.c
	$(CC) -O2 -o example-c example-c.c

example-c.c example.vcd : example sieve.hex
	./example

example : example.cpp

clean:
	rm -f example example-c *.vcd *.c *.dump *~ *\#
