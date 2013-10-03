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
  if (argc != 2) return 1;

  switch (atol(argv[1])) {
    case  1: test_mult<61>(); break;
    case  2: test_mult<64>(); break;
    case  3: test_mult< 7>(); break;
    case  4: test_mult< 8>(); break;
    case  5: test_mult< 1>(); break;
    case  6: test_mult< 2>(); break;
    case  7: test_mult< 3>(); break;
    case  8: test_mult< 4>(); break;
    case  9: test_mult< 5>(); break;
    case 10: test_mult< 6>(); break;
    default: return 1;
  }

  return 0;
}
