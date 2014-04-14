#include <cstdint>
#include <cstdlib>
#include <random>
#include <iostream>

#include "test_common.h"

using namespace std;
using namespace chdl;

template <unsigned N> void test_shl() {
  TestFunc<N>(
    [](uint64_t a, uint64_t b) { return a << (b & ((1ul<<CLOG2(N))-1)); },
    [](bvec<N> a, bvec<N> b) { return a << Zext<CLOG2(N)>(b); }
  );
}

// TODO: shr, rotr, rotl

int main(int argc, char** argv) {
  test_shl< 1>(); reset();
  test_shl< 4>(); reset();
  test_shl<16>(); reset();
  test_shl<32>(); reset();

  return 0;
}
