CXXFLAGS += -std=c++11 -O2 -I../../
LDFLAGS += -L../
LDLIBS += -lchdl

TEST_RUNS = test_adder.run test_mult.run test_logic.run test_shift.run
TESTS = test_adder test_mult test_logic test_shift

all : $(TESTS)
run : $(TEST_RUNS)

% : %.cpp test_common.h
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

%.run %.opt : %
	./$< > $@ 2> $<.opt

clean:
	rm -f $(TESTS) *~ *.run *.opt
