#include <cstdint>
#include <cstdlib>
#include <random>
#include <iostream>

#include "test_common.h"

using namespace std;
using namespace chdl;

template <unsigned N> void test_mult() {
  TestFunc<N>([](uint64_t a, uint64_t b) { return a * b; },
              [](bvec<N> a, bvec<N> b) { return Mult<N>(a, b); });
}

int main(int argc, char** argv) {
  test_mult< 1>(); reset();
  test_mult< 7>(); reset();
  test_mult<23>(); reset();
  test_mult<64>(); reset();

  return 0;
}
