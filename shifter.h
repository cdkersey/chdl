#ifndef __SHIFTER_H
#define __SHIFTER_H

#include "bvec-basic.h"
#include "mux.h"

#include "hierarchy.h"

namespace chdl {
  // Fixed shift by B bits (positive for left, negative for right). A series of
  // these could be used to construct a barrel shifter.
  template <unsigned N>
    bvec<N> ShifterStage(int B, bvec<N> in, node enable, node arith)
  {
    HIERARCHY_ENTER();

    bvec<N> shifted;
    for (int i = 0; i < N; ++i) {
      if (i+B >= 0 && i+B < N) shifted[i] = in[i+B];
      else if (B > 0)          shifted[i] = And(in[N-1], arith);
      else                     shifted[i] = Lit(0);
    }

    bvec<N> r(Mux(enable, in, shifted));
    
    HIERARCHY_EXIT();

    return r;
  }

  // 2^M bit bidirectional barrel shifter.
  template <unsigned M>
    bvec<M> Shifter(bvec<M> in, bvec<CLOG2(M)> shamt, node arith, node dir)
  {
    HIERARCHY_ENTER();

    vec<CLOG2(M)+1, bvec<M>> vl, vr;
    vl[0] = vr[0] = in;

    for (unsigned i = 0; i < CLOG2(M); ++i) {
      vl[i+1] = ShifterStage<M>(-(1<<i), vl[i], shamt[i], arith);
      vr[i+1] = ShifterStage<M>( (1<<i), vr[i], shamt[i], arith);
    }

    bvec<M> r(Mux(dir, vl[CLOG2(M)], vr[CLOG2(M)]));

    HIERARCHY_EXIT();

    return r;
  }

  template <unsigned N>
    bvec<N> operator<<(bvec<N> in, bvec<CLOG2(N)> shamt)
  {
    return Shifter(in, shamt, Lit(0), Lit(0));
  }

  template <unsigned N>
    bvec<N> operator>>(bvec<N> in, bvec<CLOG2(N)> shamt)
  {
    return Shifter(in, shamt, Lit(0), Lit(1));
  }
};

#endif
