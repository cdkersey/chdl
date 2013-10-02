#include <cstdint>
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

template <unsigned N> void test_multiplier() {
  TestFunc<N>([](uint64_t a, uint64_t b) { return a * b; },
              [](bvec<N> a, bvec<N> b) { return Mult<N>(a, b); });
}

int main(int argc, char** argv) {
  test_adder<61>();
  //test_multiplier<61>();

  return 0;
}
