#ifndef __ENC_H
#define __ENC_H

#include <iostream>

#include "gates.h"
#include "bvec-basic.h"
#include "shifter.h"
#include "hierarchy.h"

namespace chdl {
  static bvec<1> PriEnc(node &valid, const bvec<2> &in, bool reverse=false) {
    HIERARCHY_ENTER();
    valid = in[0] || in[1];
    node ret;
    if (reverse) ret = !in[0];
    else         ret = in[1];
    HIERARCHY_EXIT();

    return ret;
  }

  template <unsigned N>
    bvec<CLOG2(N)> PriEnc(node &valid, const bvec<N> &in, bool reverse = false)
  {
    HIERARCHY_ENTER();

    const unsigned A(N/2), B(N - A);

    bvec<A> rin(in[range<0, A-1>()]);
    node rvalid;
    bvec<CLOG2(A)> r(PriEnc(rvalid, rin, reverse));

    bvec<B> lin(in[range<A, N-1>()]);
    node lvalid;
    bvec<CLOG2(A)> l(Zext<CLOG2(A)>(PriEnc(lvalid, lin, reverse)));

    valid = lvalid || rvalid;

    bvec<CLOG2(N)> ret;

    if (reverse)
      ret = Cat(!rvalid, Mux(rvalid, l, r));
    else
      ret = Cat(lvalid, Mux(lvalid, r, l));
 
    HIERARCHY_EXIT();
 
    return ret;
  }

  // Integer logarithm unit (find index of highest set bit); priority encoder
  template <unsigned N> bvec<CLOG2(N)> Log2(bvec<N> x) {
    HIERARCHY_ENTER();

    node valid;
    bvec<CLOG2(N)> ret(PriEnc(valid, x));

    HIERARCHY_EXIT();

    return ret;
  }

  template <unsigned N> bvec<CLOG2(N)> Lsb(bvec<N> x) {
    return PriEnc(valid, x, true);
  }

  // Cheap encoder, results valid only when exactly one input set
  template <unsigned N> bvec<CLOG2(N)> Enc(bvec<N> x) {
    HIERARCHY_ENTER();

    vec<CLOG2(N), bvec<N>> mask;
    for (unsigned i = 0; i < CLOG2(N); ++i)
      for (unsigned j = 0; j < N; ++j)
        mask[i][j] = Lit(j & (1 << i));

    bvec<CLOG2(N)> out;
    for (unsigned i = 0; i < CLOG2(N); ++i)
      out[i] = OrN(mask[i] & x);

    HIERARCHY_EXIT();

    return out;
  }
};

#endif
