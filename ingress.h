#ifndef __CHDL_INGRESS_H
#define __CHDL_INGRESS_H

#include "chdl.h"
#include "nodeimpl.h"
#include "sim.h"

#include <functional>

namespace chdl {
  template <typename T> node IngressFunc(const T &f);
  static node Ingress(bool &x);
  static node IngressAutoclear(bool &x);
  template <unsigned N, typename T> void IngressInt(bvec<N> &b, const T &x);
  template <unsigned N, typename T> bvec<N> IngressInt(const T &x);
  template <unsigned N, typename T> void IngressIntFunc(bvec<N> &b, const T &x);
  template <unsigned N, typename T> bvec<N> IngressIntFunc(const T &x);

  template <typename T> class ingressimpl : public nodeimpl {
  public:
    ingressimpl(T f): nodeimpl(), f(f), eval_time(0) {} 
    bool eval() {
      if (sim_time() >= eval_time) { val = f(); eval_time = sim_time() + 1; }
      return val;
    }

    void print(std::ostream &out) { abort(); }
    void print_vl(std::ostream &out) { abort(); }
  private:
    cycle_t eval_time;
    bool val;
    T f;
  };

  template <typename T> ingressimpl<T> *IngressImpl(T f) {
    return new ingressimpl<T>(f);
  }

  template<typename T> node IngressFunc(const T &f) {
    IngressImpl(f);
    return node(nodes.size()-1);
  }

  static node Ingress(bool &x) {
    return IngressFunc([&x](){ return x; });
  }

  static node IngressAutoclear(bool &x) {
    return IngressFunc([&x](){ bool prevx(x); x = false; return prevx; });
  }

  template <typename T> std::function<bool()> GetBit(unsigned bit, T &x) {
    return std::function<bool()>([&x, bit](){ return bool((x >> bit)&1); });
  }

  template <typename T> std::function<bool()> GetBitFunc(unsigned bit, T &x) {
    return std::function<bool()>([&x, bit](){ return bool((x() >> bit)&1); });
  }

  template <unsigned N, typename T> void IngressInt(bvec<N> &b, T &x) {
    for (unsigned i = 0; i < N; ++i)
      b[i] = IngressFunc(GetBit(i, x));
  }

  template <unsigned N, typename T> void IngressIntFunc(bvec<N> &b, const T &x)
  {
    for (unsigned i = 0; i < N; ++i)
      b[i] = IngressFunc(GetBitFunc(i, x));
  }

  template <unsigned N, typename T> bvec<N> IngressInt(T &x) {
    bvec<N> r;
    IngressInt(r, x);
    return r;
  }

  template <unsigned N, typename T> bvec<N> IngressIntFunc(const T& x) {
    bvec<N> r;
    IngressIntFunc(r, x);
    return r;
  }
}

#endif
