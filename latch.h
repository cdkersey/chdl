#ifndef __LATCH_H
#define __LATCH_H

#include <set>

#include "node.h"
#include "bvec-basic.h"

namespace chdl {
  template <typename T> T Latch(node l, const T &d) {
    return Mux(l, d, Wreg(Inv(l), d));
  }
};

#endif
