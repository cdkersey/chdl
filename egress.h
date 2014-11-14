#ifndef __CHDL_EGRESS_H
#define __CHDL_EGRESS_H
#include "nodeimpl.h"
#include "cdomain.h"
#include "tickable.h"
#include "tap.h"

#include <functional>

namespace chdl {
  template <typename T> void EgressFunc(const T& f, node n);
  static void Egress(bool &b, node n);
  template <typename T, unsigned N> void EgressInt(T &i, bvec<N> bv);

  template <typename T> class egress : public tickable {
  public:
    egress(const T& x, node n): n(n), x(x) { gtap(n); }
    virtual ~egress() {}

    void pre_tick(evaluator_t &e) { x(e(n)); }
  private:
    node n;
    T x;
  };
}

template <typename T> void chdl::EgressFunc(const T &f, chdl::node n) {
  new chdl::egress<T>(f, n);
}

static void chdl::Egress(bool &b, chdl::node n) {
  chdl::EgressFunc([&b](bool val){ b = val; }, n);
}

template <typename T, unsigned N> void chdl::EgressInt(T &x, chdl::bvec<N> bv)
{
  using namespace chdl;
  using namespace std;

  x = 0;

  for (unsigned i = 0; i < N; ++i)
    chdl::EgressFunc(
      [i, &x](bool val){
        if (val) x |= (1ull<<i); else x &= ~(1ull<<i);
      },
      bv[i]
    );
}
#endif
