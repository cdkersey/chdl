#ifndef __SHIFTER_H
#define __SHIFTER_H

#include "bvec-basic.h"
#include "mux.h"

namespace chdl {
  // Fixed shift by B bits (positive for left, negative for right). A series of
  // these could be used to construct a barrel shifter.
  template <unsigned N>
    bvec<N> ShifterStage(int B, bvec<N> in, node enable, node arith)
  {
    bvec<N> shifted;
    for (int i = 0; i < N; ++i) {
      if (i+B >= 0 && i+B < N) shifted[i] = in[i+B];
      else if (B > 0)          shifted[i] = And(in[N-1], arith);
      else                     shifted[i] = Lit(0);
    }

    return Mux(enable, in, shifted);
  }

  // 2^M bit bidirectional barrel shifter.
  template <unsigned M>
    bvec<1<<M> Shifter(bvec<1<<M> in, bvec<M> shamt, node arith, node dir)
  {
    const unsigned SIZE(1<<M);
    vec<M+1, bvec<SIZE>> vl, vr;
    vl[0] = vr[0] = in;

    for (unsigned i = 0; i < M; ++i) {
      vl[i+1] = ShifterStage<SIZE>(-(1<<i), vl[i], shamt[i], arith);
      vr[i+1] = ShifterStage<SIZE>( (1<<i), vr[i], shamt[i], arith);
    }

    return Mux(dir, vl[M], vr[M]);
  }

  template <unsigned M>
    bvec<1<<M> operator<<(bvec<1<<M> in, bvec<M> shamt)
  {
    return Shifter(in, shamt, Lit(0), Lit(0));
  }

  template <unsigned M>
    bvec<1<<M> operator>>(bvec<1<<M> in, bvec<M> shamt)
  {
    return Shifter(in, shamt, Lit(0), Lit(1));
  }
};

#endif
