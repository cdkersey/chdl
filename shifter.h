#ifndef __SHIFTER_H
#define __SHIFTER_H

#include "bvec-basic.h"
#include "mux.h"

#define LOG2(x) ((unsigned)(!!(0xffff0000&(x))<<4) | (!!(0xff00ff00&(x))<<3) \
              | (!!(0xf0f0f0f0&(x))<<2) | (!!(0xcccccccc&(x))<<1) \
              |  !!(0xaaaaaaaa&(x)) )


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
    bvec<M> Shifter(bvec<M> in, bvec<LOG2(M)> shamt, node arith, node dir)
  {
    vec<LOG2(M)+1, bvec<M>> vl, vr;
    vl[0] = vr[0] = in;

    for (unsigned i = 0; i < LOG2(M); ++i) {
      vl[i+1] = ShifterStage<M>(-(1<<i), vl[i], shamt[i], arith);
      vr[i+1] = ShifterStage<M>( (1<<i), vr[i], shamt[i], arith);
    }

    return Mux(dir, vl[LOG2(M)], vr[LOG2(M)]);
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
