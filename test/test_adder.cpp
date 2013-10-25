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
  test_adder< 1>(); reset();
  test_adder< 3>(); reset();
  test_adder< 7>(); reset();
  test_adder< 8>(); reset();
  test_adder<13>(); reset();
  test_adder<16>(); reset();
  test_adder<19>(); reset();
  test_adder<33>(); reset();
  test_adder<64>(); reset();

  return 0;
}
