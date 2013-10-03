#include <cstdint>
#include <cstdlib>
#include <random>
#include <iostream>

#include "test_common.h"

using namespace std;
using namespace chdl;

template <unsigned N> void test_and() {
  TestFunc<N>([](uint64_t a, uint64_t b) { return a & b; },
              [](bvec<N> a, bvec<N> b) { return a & b; });
}

template <unsigned N> void test_or() {
  TestFunc<N>([](uint64_t a, uint64_t b) { return a | b; },
              [](bvec<N> a, bvec<N> b) { return a | b; });
}

template <unsigned N> void test_xor() {
  TestFunc<N>([](uint64_t a, uint64_t b) { return a ^ b; },
              [](bvec<N> a, bvec<N> b) { return a ^ b; });
}

template <unsigned N> void test_not() {
  TestFunc<N>([](uint64_t a, uint64_t b) { return ~a; },
              [](bvec<N> a, bvec<N> b) { return ~a; });
}

int main(int argc, char** argv) {
  if (argc != 2) return 1;

  switch (atol(argv[1])) {
    case  1: test_and<61>(); break;
    case  2: test_or<64>(); break;
    case  3: test_xor<61>(); break;
    case  4: test_not<64>(); break;
    case  5: test_and<61>(); break;
    case  6: test_or<64>(); break;
    case  7: test_xor<63>(); break;
    case  8: test_not<17>(); break;
    case  9: test_and<9>(); break;
    case 10: test_or<4>(); break;
    default: return 1;
  }

  return 0;
}
