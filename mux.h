#ifndef __MUX_H
#define __MUX_H

#include <iostream>

#include "gates.h"
#include "bvec-basic.h"

#include "hierarchy.h"

namespace chdl {
  constexpr unsigned LOG2(unsigned long x) {
    return x <= 1 ? 0 : LOG2(x >> 1) + 1;
  }

  constexpr unsigned CLOG2(unsigned long x) {
    return x&(x-1) ? LOG2(x) + 1 : LOG2(x);
  }

  // 2-input N-bit mux
  template <typename T> T Mux(node sel, const T &i0, const T &i1) {
    HIERARCHY_ENTER();
    T o;
    for (unsigned i = 0; i < sz<T>::value; ++i)
      Flatten(o)[i] = Mux(sel, Flatten(i0)[i], Flatten(i1)[i]);
    HIERARCHY_EXIT();
    return o;
  }

  // Base cases for recursive mux
  template <typename T> T Mux(bvec<0> sel, const vec<1, T> &in) {
    return in[0];
  } 

  template <typename T> T Mux(bvec<1> sel, const vec<2, T> &in) {
    return Mux(sel[0], in[0], in[1]);
  }

  // 2^M-input recursive mux
  template <unsigned N, typename T>
    T Mux(bvec<CLOG2(N)> sel, const vec<N, T>&in)
  {
    HIERARCHY_ENTER();
    const unsigned LSZ = (1<<(CLOG2(N)-1)), RSZ = (N - LSZ);
    vec<LSZ, T> lv(in[range<0,LSZ-1>()]);
    vec<RSZ, T> rv(in[range<LSZ,N-1>()]);
    
    T l(Mux(sel[range<0,CLOG2(LSZ)-1>()], lv)), 
      r(Mux(sel[range<0,CLOG2(RSZ)-1>()], rv)),
      out(Mux(sel[CLOG2(N)-1], l, r));
    HIERARCHY_EXIT();

    return out;
  }

  // 1-2 decoder/demux
  static inline bvec<2> Decoder(node i, node e = Lit(1)) {
    HIERARCHY_ENTER();
    bvec<2> out{And(Inv(i), e), And(i, e)};
    HIERARCHY_EXIT();

    return out;
  }

  // M-2^M decoder/demux
  template <unsigned M> bvec<(1<<M)> Decoder(bvec<M> sel, node e = Lit(1)) {
    HIERARCHY_ENTER();
    bvec<2*(1<<M)> nodes;
    nodes[1] = e;
    for (unsigned i = 1; i < (1<<M); ++i) {
      bvec<2> r = Decoder(sel[M - LOG2(i) - 1], nodes[i]);
      nodes[2*i] = r[0];
      nodes[2*i+1] = r[1];
    }
    HIERARCHY_EXIT();

    return nodes[range<(1<<M),2*(1<<M)-1>()];
  }
};

#endif
