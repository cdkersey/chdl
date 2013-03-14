#ifndef __ENC_H
#define __ENC_H

#include <iostream>

#include "gates.h"
#include "bvec-basic.h"
#include "shifter.h"
#include "hierarchy.h"

namespace chdl {
  // Integer logarithm unit (find index of highest set bit); priority encoder
  template <unsigned N> bvec<CLOG2(N)> Log2(bvec<N> x) {
    HIERARCHY_ENTER();

    // First, produce the masks.
    vec<N, bvec<N>> mask;
    for (unsigned i = N/2; i > 0; i /= 2)
      for (unsigned j = 0; j < N; ++j)
        mask[i][j] = Lit(j%(i*2) >= i);

    // Next, produce the masking chain.
    bvec<CLOG2(N)> out;
    vec<N+1, bvec<N>> val; val[N] = x;
    for (unsigned i = N/2; i > 0; i /= 2) {
      out[log2(i)] = OrN(mask[i] & val[i*2]);
      val[i] = Mux(out[log2(i)], val[i*2], val[i*2] & mask[i]);
    }

    HIERARCHY_EXIT();

    return out;
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
