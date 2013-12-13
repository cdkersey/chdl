CXXFLAGS = -std=c++11 -O2
LDLIBS = -lchdl

TEST_RUNS = test_adder.run test_mult.run test_logic.run
TESTS = test_adder test_mult test_logic

all : $(TESTS)
run : $(TEST_RUNS)

test_adder: test_adder.cpp test_common.h
test_mult: test_mult.cpp test_common.h
test_logic: test_logic.cpp test_common.h

%.run %.opt : %
	./$< > $@ 2> $<.opt

clean:
	rm -f $(TESTS) *~ *.run *.opt
