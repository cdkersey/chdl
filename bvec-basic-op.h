// A bvec is a fixed-length vector of bits. rvecs are like bvecs with a connect
// function.
#ifndef __BVEC_BASIC_OP_H
#define __BVEC_BASIC_OP_H

#include "bvec-basic.h"

namespace chdl {
  // Fundamental bitwise logic
  template <unsigned N>
    bvec<N> operator~(bvec<N> in) { return Not(in); }
  template <unsigned N>
    bvec<N> operator&(bvec<N> a, bvec<N> b) { return And(a, b); }
  template <unsigned N>
    bvec<N> operator|(bvec<N> a, bvec<N> b) { return Or(a, b); }
  template <unsigned N>
    bvec<N> operator^(bvec<N> a, bvec<N> b) { return Xor(a, b); }

  // Equality detection, re-implemented from bvec-basic.h to demonstrate the
  // simplicity of using bit vectors.
  template <unsigned N>
    node operator==(bvec<N> a, bvec<N> b) { return AndN(~(a ^ b)); }
};

#endif
