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
  template <unsigned N> bvec<N> Mux(node sel, bvec<N> i0, bvec<N> i1) {
    HIERARCHY_ENTER();
    bvec<N> o;
    for (unsigned i = 0; i < N; ++i)
      o[i] = Mux(sel, i0[i], i1[i]);
    HIERARCHY_EXIT();
    return o;
  }

  // 2^M to 1 mux
  template <unsigned M>
    node Mux(bvec<M> sel, bvec<1<<M> inputs)
  {
    HIERARCHY_ENTER();
    bvec<2<<M> nodes;
    nodes[range<(1<<M),2*(1<<M)-1>()] = inputs;
    for (int i = (1<<M)-1; i >= 1; --i)
      nodes[i] = Mux(sel[M - LOG2(i) - 1], nodes[i*2], nodes[i*2 + 1]);
    HIERARCHY_EXIT();
    return nodes[1];
  }

  // 2^M-input recursive mux
  template <unsigned N, unsigned M, typename T>
    vec<M, T> Mux(bvec<CLOG2(N)> sel, const vec<N, vec<M, T> > &in)
  {
    vec<M, T> out;
    for (unsigned i = 0; i < M; ++i) {
      vec<N, T> inx;
      for (unsigned j = 0; j < N; ++j) inx[j] = in[j][i];
      out[i] = Mux(sel, inx);
    }
 
    return out;
  }

  // 1-2 decoder/demux
  static inline bvec<2> Decoder(node i, node e = Lit(1)) {
    HIERARCHY_ENTER();
    bvec<2> out;
    out[0] = And(Inv(i), e);
    out[1] = And(i, e);
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
