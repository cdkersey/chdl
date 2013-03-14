// Instantiate an N-bit Wallace tree multiplier
#ifndef __MULT_H
#define __MULT_H

#include <vector>
#include <sstream>

#include "bvec.h"
#include "bvec-basic.h"
#include "bvec-basic-op.h"
#include "adder.h"

#include "hierarchy.h"

namespace chdl {
  // Shift input left by L bits
  template <unsigned N> bvec<N> ShiftLeft(bvec<N> in, unsigned l) {
    bvec<N> out;
    for (unsigned i = 0; i < l; ++i) out[i] = Lit(0);
    for (unsigned i = l; i < N; ++i) out[i] = in[i - l];
    return out;
  }

  void HalfAdder(node &sum, node &carry, node a, node b) {
    HIERARCHY_ENTER();
    using namespace std;
    sum = Xor(a, b);
    carry = a && b;
    HIERARCHY_EXIT();
  }

  void FullAdder(node &sum, node &carry, node a, node b, node c) {
    HIERARCHY_ENTER();
    using namespace std;
    node p1 = Xor(a, b);
    sum = Xor(c, p1);
    carry = a && b || p1 && c; 
    HIERARCHY_EXIT();
  }

  template <unsigned N> bvec<N> Mult(bvec<N> a, bvec<N> b) {
    HIERARCHY_ENTER();
    using namespace std;
    vector<vector<node>> terms(N);
    
    // Find initial value of terms.
    for (unsigned j = 0; j < N; ++j)
      for (unsigned i = 0; i < N; ++i)
        terms[j].push_back(ShiftLeft(a, i)[j] && b[i]);

    // Implement the Wallace tree.
    bool final;
    for (;;) {
      final = true;
      vector<vector<node>> next(N);
      for (unsigned j = 0; j < terms.size(); ++j)
        if (terms[j].size() > 2) final = false;
      if (final) break;

      for (unsigned j = 0; j < terms.size(); ++j) {
        for (unsigned i = 0; i < terms[j].size();) {
          if (terms[j].size() - i >= 3) {
            node s, c;
            FullAdder(s, c, terms[j][i], terms[j][i+1], terms[j][i+2]);
            next[j].push_back(s);
            if (j + 1 < N) next[j+1].push_back(c);
            i += 3;
          } else if (terms[j].size() - i >= 2) {
            node s, c;
            HalfAdder(s, c, terms[j][i], terms[j][i+1]);
            next[j].push_back(s);
            i += 2;
            if (j + 1 < N) next[j+1].push_back(c);
          } else if (terms[j].size() - i == 1) {
            next[j].push_back(terms[j][i]);
            ++i;
          }
        }
      }

      terms.clear();
      terms = next;
    }

    // Implement the final adder.
    bvec<N> term1, term2, prod;
    for (unsigned i = 0; i < N; ++i) {
      if (terms[i].size() == 0) {
        term1[i] = term2[i] = Lit(0);
      } else if (terms[i].size() == 1) {
        term1[i] = terms[i][0];
        term2[i] = Lit(0);
      } else { // (terms[i].size() == 2)
        term1[i] = terms[i][0];
        term2[i] = terms[i][1];
      }
    }
    prod = term1 + term2;

    HIERARCHY_EXIT();

    return prod;
  }

  template <unsigned N>
    bvec<N> operator*(bvec<N> a, bvec<N> b) { return Mult<N>(a, b); }
};
#endif
