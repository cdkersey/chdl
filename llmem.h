#ifndef CHDL_LLMEM_H
#define CHDL_LLMEM_H

#include <fstream>
#include <string>
#include <vector>

#include "bvec-basic.h"
#include "node.h"
#include "lit.h"
#include "reg.h"
#include "mux.h"
#include "ttable.h"

#include "hierarchy.h"

namespace chdl {
  // For now, LLRam has no initialization.
  template <unsigned M, unsigned N, unsigned R>
    vec<R, bvec<N>> LLRam(vec<R, bvec<M>> qa, bvec<N> d, bvec<M> da, node w)
  {
    HIERARCHY_ENTER();

    bvec<1<<M> wrsig(Decoder(da, w));

    vec<1<<M, bvec<N>> bits;
    for (unsigned i = 0; i < 1<<M; ++i)
      bits[i] = Wreg(wrsig[i], d);
    
    vec<R, bvec<N>> r;
    for (unsigned i = 0; i < R; ++i) r[i] = Mux(qa[i], bits);

    HIERARCHY_EXIT();

    return r;
  }

  // Single-read-port LLRam
  template <unsigned M, unsigned N>
    bvec<N> LLRam(bvec<M> qa, bvec<N> d, bvec<M> da, node w)
  {
    vec<1, bvec<M>> qav(qa);
    return LLRam(qav, d, da, w)[0];
  }

  template <unsigned M, unsigned N>
    bvec<N> LLRam(bvec<M> a, bvec<N> d, node w)
  {
    return LLRam(a, d, a, w);
  }

};

#endif
