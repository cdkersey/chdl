#include <cstdint>
#include <cstdlib>
#include <random>
#include <iostream>

#include "test_common.h"

using namespace std;
using namespace chdl;

// Test an N-bit adder across t trials, where N is <= 64
template <unsigned N> void test_adder() {
  TestFunc<N>([](uint64_t a, uint64_t b) { return a + b; },
              [](bvec<N> a, bvec<N> b) { return a + b; });
}

int main(int argc, char** argv) {
  if (argc != 2) return 1;

  switch (atol(argv[1])) {
    case  1: test_adder<61>(); break;
    case  2: test_adder<64>(); break;
    case  3: test_adder<17>(); break;
    case  4: test_adder< 8>(); break;
    case  5: test_adder< 1>(); break;
    case  6: test_adder<19>(); break;
    case  7: test_adder<24>(); break;
    case  8: test_adder<32>(); break;
    case  9: test_adder<12>(); break;
    case 10: test_adder<31>(); break;
    default: return 1;
  }

  return 0;
}
