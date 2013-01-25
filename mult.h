// Instantiate an N-bit Wallace tree multiplier
#ifndef __MULT_H
#define __MULT_H

#include <vector>

#include "bvec.h"
#include "bvec-basic.h"
#include "bvec-basic-op.h"
#include "adder.h"

namespace chdl {
  // Shift input left by L bits
  template <unsigned N> bvec<N> ShiftLeft(bvec<N> in, unsigned l) {
    bvec<N> out;
    for (unsigned i = 0; i < l; ++i) out[i] = Lit(0);
    for (unsigned i = l; i < N; ++i) out[i] = in[i - l];
    return out;
  }

  void HalfAdder(node &sum, node &carry, node a, node b) {
    sum = Xor(a, b);
    carry = a && b;
  }

  void FullAdder(node &sum, node &carry, node a, node b, node c) {
    node p1 = Xor(a, b);
    sum = Xor(c, p1);
    carry = a && b || p1 && c; 
  }

  template <unsigned N> bvec<N> WallaceMult(bvec<N> a, bvec<N> b) {
    // TODO: Implement me!
  }

  template <unsigned N> bvec<N> Mult(bvec<N> a, bvec<N> b) {
    using namespace std;
    // First find the root terms
    vector<bvec<N>> terms(N*2);
    for (unsigned j = 0; j < N; ++j)
      for (unsigned i = 0; i < N; ++i)
        terms[j+N][i] = ShiftLeft(a, j)[i] && b[j];

    // Then add together the terms
    for (unsigned i = N-1; i >= 1; --i)
      terms[i] = terms[i*2] + terms[i*2 + 1];

    // Return the sum of all of the terms.
    return terms[1];
  }
};
#endif
