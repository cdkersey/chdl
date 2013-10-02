CXXFLAGS = -std=c++11 -O2
LDLIBS = -lchdl

TEST_RUNS = test_adder.run
TESTS = test_adder

all : $(TESTS)
run : $(TEST_RUNS)

test_adder: test_adder.cpp test_common.h

%.run : %
	./$< > $@

clean:
	rm -f $(TESTS) *~ *.run

