#include <cstdint>
#include <cstdlib>
#include <random>
#include <iostream>

#include "test_common.h"

using namespace std;
using namespace chdl;

template <unsigned N> unsigned long roR(unsigned long a, unsigned b) {
  b %= N;
  return (a >> b) | a << (N - b);
}

template <unsigned N> unsigned long roL(unsigned long a, unsigned b) {
  b %= N;
  return (a << b) | a >> (N - b);
}

template <unsigned N> void test_shl() {
  TestFunc<N>(
    [](uint64_t a, uint64_t b) { return a << (b & ((1ul<<CLOG2(N))-1)); },
    [](bvec<N> a, bvec<N> b) { return a << Zext<CLOG2(N)>(b); }
  );
}

template <unsigned N> void test_shr() {
  TestFunc<N>(
    [](uint64_t a, uint64_t b) { return a >> (b & ((1ul<<CLOG2(N))-1)); },
    [](bvec<N> a, bvec<N> b) { return a >> Zext<CLOG2(N)>(b); }
  );
}

template <unsigned N> void test_ror() {
  TestFunc<N>(
    [](uint64_t a, uint64_t b) { return roR<N>(a, b); },
    [](bvec<N> a, bvec<N> b) { return RotR(a, Zext<CLOG2(N)>(b)); }
  );
}

template <unsigned N> void test_rol() {
  TestFunc<N>(
    [](uint64_t a, uint64_t b) { return roL<N>(a, b); },
    [](bvec<N> a, bvec<N> b) { return RotL(a, Zext<CLOG2(N)>(b)); }
  );
}

template <unsigned N> void test_shift() {
  test_shr<N>(); reset();
  test_shl<N>(); reset();
  test_ror<N>(); reset();
  test_rol<N>(); reset();
  test_ror<N>(); reset();
}

int main(int argc, char** argv) {
  test_shift< 1>();
  test_shift< 4>();
  test_shift<16>();
  test_shift<32>();

  return 0;
}
