#ifndef __SHIFTER_H
#define __SHIFTER_H

#include "bvec-basic.h"
#include "mux.h"

#include "hierarchy.h"

#define  LOG2_INT_1(x) 0
#define  LOG2_INT_2(x) ((x&       0x2)?LOG2_INT_1 (x>> 1)+ 1: LOG2_INT_1(x))
#define  LOG2_INT_4(x) ((x&       0xc)?LOG2_INT_2 (x>> 2)+ 2: LOG2_INT_2(x))
#define  LOG2_INT_8(x) ((x&      0xf0)?LOG2_INT_4 (x>> 4)+ 4: LOG2_INT_4(x))
#define LOG2_INT_16(x) ((x&    0xff00)?LOG2_INT_8 (x>> 8)+ 8: LOG2_INT_8(x))
#define LOG2_INT_32(x) ((x&0xffff0000)?LOG2_INT_16(x>>16)+16:LOG2_INT_16(x))

#define LOG2(x) LOG2_INT_32((x))
#define CLOG2(x) (LOG2_INT_32((x)) + (((x)&((x)-1)) != 0))

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
