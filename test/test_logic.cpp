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

template <unsigned N> void test_all() {
  test_and<N>();
  test_or<N>();
  test_xor<N>();
  test_not<N>();
}

int main(int argc, char** argv) {
  test_all< 1>(); reset();
  test_all< 3>(); reset();
  test_all< 7>(); reset();
  test_all<11>(); reset();
  test_all<13>(); reset();
  test_all<17>(); reset();
  test_all<19>(); reset();
  test_all<23>(); reset();
  test_all<64>(); reset();

  return 0;
}
