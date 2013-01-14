// Instantiate an N-bit Kogge-Stone adder
#ifndef __ADDER_H
#define __ADDER_H

#include <vector>

#include "bvec.h"

namespace chdl {
  template <unsigned N> bvec<N> Adder(bvec<N> a, bvec<N> b, node cin = Lit(0)) {
    std::vector<bvec<N+1>> g(log2(N)+3), p(log2(N)+3), i(log2(N)+3);
    bvec<N> s;

    g[0][0] = cin;
    p[0][0] = Lit(0);

    for (unsigned j = 0; j < N; ++j) p[0][j+1] = Xor(a[j], b[j]);
    for (unsigned j = 0; j < N; ++j) g[0][j+1] = a[j] && b[j];

    unsigned k;
    for (k = 0; (1l<<k) < 2*N; ++k) {
      for (unsigned j = 0; j < N+1; ++j) {
        if (j < (1l<<k)) {
          g[k+1][j] = g[k][j];
          p[k+1][j] = p[k][j];
        } else {
          i[k+1][j] = p[k][j] && g[k][j - (1l<<k)];
          g[k+1][j] = i[k+1][j] || g[k][j];
          p[k+1][j] = p[k][j] && p[k][j-(1l<<k)];
        }
      }
    }

    for (unsigned j = 0; j < N; ++j) s[j] = Xor(p[0][j+1], g[k][j]);

    return s;
  }

  // This is the worst; there has to be someway to counter this but keep bvec's
  // generality. Perhaps the answer is autoconversion to yet another type.
  template <unsigned N>
    bvec<N> operator+(bvec<N> a, bvec<N> b) { return Adder<N>(a, b); }
};
#endif
