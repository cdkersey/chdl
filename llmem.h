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

namespace chdl {
  template <unsigned M, unsigned N>
    bvec<N> LLRom(bvec<M> a, std::string filename)
  {
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

    return Mux(a, bits);
  }
};

#endif
