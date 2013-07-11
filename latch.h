#ifndef __LATCH_H
#define __LATCH_H

#include <set>

#include "node.h"
#include "bvec-basic.h"

namespace chdl {
  node Latch(node l, node d);

  template <unsigned N> bvec<N> Latch(node l, bvec<N> d) {
    bvec<N> r;
    for (unsigned i = 0; i < N; ++i)
      r[i] = Latch(l, d[i]);
    return r;
  }
};

#endif
