CXXFLAGS = -std=c++11 -O2
LDLIBS = -lchdl

TEST_RUNS = test_adder.run test_mult.run test_logic.run
TESTS = test_adder test_mult test_logic

all : $(TESTS)
run : $(TEST_RUNS)

test_adder: test_adder.cpp test_common.h
test_mult: test_mult.cpp test_common.h

%.run : %
	./$< 1 > $@ 2>> opt.log
	./$< 2 >> $@ 2>> opt.log
	./$< 3 >> $@ 2>> opt.log
	./$< 4 >> $@ 2>> opt.log
	./$< 5 >> $@ 2>> opt.log
	./$< 6 >> $@ 2>> opt.log
	./$< 7 >> $@ 2>> opt.log
	./$< 8 >> $@ 2>> opt.log
	./$< 9 >> $@ 2>> opt.log
	./$< 10 > $@ 2>> opt.log

clean:
	rm -f $(TESTS) *~ *.run opt.log
