#ifndef __LLMEM_H
#define __LLMEM_H

#include <fstream>
#include <string>
#include <vector>

#include "bvec-basic.h"
#include "node.h"
#include "lit.h"
#include "reg.h"
#include "mux.h"

#include "hierarchy.h"

namespace chdl {
  template <unsigned M, unsigned N>
    bvec<N> LLRom(bvec<M> a, std::string filename)
  {
    HIERARCHY_ENTER();

    using namespace std;
    vector<bool> contents(N<<M);

    ifstream in(filename.c_str());
    size_t i = 0;
    while (i < (1ull<<M) && in) {
      unsigned long long val;
      in >> hex >> val;
      for (unsigned j = 0; j < N; ++j) {
        contents[i*N + j] = val & 1;
        val >>= 1;
      }
      ++i;
    }

    vec<1ull<<M, bvec<N>> bits;
    for (size_t i = 0; i < 1ull<<M; ++i)
      for (unsigned j = 0; j < N; ++j)
        bits[i][j] = Lit(contents[i*N + j]);

    bvec<N> r(Mux(a, bits));

    HIERARCHY_EXIT();

    return r;
  }

  // For now, RAM has no initialization.
  template <unsigned M, unsigned N>
    bvec<N> LLRam(bvec<M> qa, bvec<N> d, bvec<M> da, node w)
  {
    HIERARCHY_ENTER();

    bvec<1<<M> wrsig(Decoder(da, w));

    vec<1<<M, bvec<N>> bits;
    for (unsigned i = 0; i < 1<<M; ++i)
      bits[i] = Wreg(wrsig[i], d);
    
    bvec<N> r(Mux(qa, bits));

    HIERARCHY_EXIT();

    return r;
  }

  template <unsigned M, unsigned N>
    bvec<N> LLRam(bvec<M> a, bvec<N> d, node w)
  {
    return LLRam(a, d, a, w);
  }

};

#endif
