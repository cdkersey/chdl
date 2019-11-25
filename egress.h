#ifndef CHDL_EGRESS_H
#define CHDL_EGRESS_H
#include "nodeimpl.h"
#include "cdomain.h"
#include "tickable.h"
#include "tap.h"

#include <functional>

namespace chdl {
  template <typename T> void EgressFunc(const T& f, node n, bool early=false);
  static void Egress(bool &b, node n);
  template <typename T, unsigned N> void EgressInt(T &i, bvec<N> bv);

  template <typename T> class egress : public tickable {
  public:
    egress(const T& x, node n, bool early = false):
      n(n), x(x), early(early)
      { gtap(n); }

    virtual ~egress() {}

    void pre_tick(cdomain_handle_t cd) { if (early) x(nodes[n]->eval(cd)); }
    void tick(cdomain_handle_t cd) { if (!early) x(nodes[n]->eval(cd)); }

  private:
    bool early;

    node n;
    T x;
  };
}

template <typename T>
  void chdl::EgressFunc(const T &f, chdl::node n, bool early)
{
  new chdl::egress<T>(f, n, early);
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
      bv[i], true
    );
}
#endif
