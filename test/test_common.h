#include <random>

#include "../chdl.h"
#include "../ingress.h"
#include "../egress.h"

const unsigned TRIALS(1000);

using namespace std;
using namespace chdl;

template<unsigned N> uint64_t RandInt() {
  static default_random_engine e(0);
  uniform_int_distribution<uint64_t> d(0,(1ul<<N)-1);
  return d(e);
}

template <unsigned N> uint64_t Trunc(uint64_t x) {
  uint64_t mask((1ul<<N)-1);
  return x & mask;
}

// Test an N-bit adder across t trials, where N is <= 64
template <unsigned N, typename F, typename H>
  void TestFunc(const F &f, const H &h)
{
  static uint64_t out, in_a, in_b;

  bvec<N> a(IngressInt<N>(in_a)), b(IngressInt<N>(in_b)), s(h(a,b));
  EgressInt(out, s);

  for (unsigned i = 0; i < TRIALS; ++i) {
    in_a = RandInt<N>();
    in_b = RandInt<N>();
    uint64_t sum(Trunc<N>(f(in_a, in_b)));
    advance();
    if (out != sum) {
      cout << "func(" << in_a << ", " << in_b << ") => " << sum
           << "; incorrect value " << out << endl;
      abort();
    } else {
      cout << in_a << ", " << in_b << ": " << sum << endl;
    }
  }
}

