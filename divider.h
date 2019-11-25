#ifndef __DIVIDER_H
#define __DIVIDER_H

#include "bvec.h"
#include "bvec-basic.h"
#include "bvec-basic-op.h"
#include "shifter.h"
#include "adder.h"
#include "hierarchy.h"

namespace chdl {
  template <unsigned N>
    bvec<N> Div(bvec<N> a, bvec<N> b, bvec<N> mod = Lit<N>(0))
  {
    HIERARCHY_ENTER();
    bvec<N> q;
    vec<N + 1, bvec<N>> rem;
    rem[0] = a;
    for (unsigned i = 0; i < N; ++i) {
      unsigned bpos(N - i - 1);
      bvec<2*N> big_b(Cat(Lit<N>(0), b) << Lit<CLOG2(N)+1>(bpos));
      q[bpos] = big_b <= Cat(Lit<N>(0), rem[i]);
      bvec<N> dif = rem[i] - (b << Lit<CLOG2(N)>(bpos));
      rem[i+1] = Mux(q[bpos], rem[i], dif);
    }

    mod = rem[N];
    HIERARCHY_EXIT();
    return q;
  }

  template <unsigned N> bvec<N> operator/(bvec<N> a, bvec<N> b) {
    return Div(a, b);
  }

  template <unsigned N> bvec<N> operator%(bvec<N> a, bvec<N> b) {
    bvec<N> r;
    Div(a, b, r);
    return r;
  }
};

#endif
