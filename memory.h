#ifndef __MEMORY_H
#define __MEMORY_H

#include <string>
#include <vector>

#include "bvec-basic.h"
#include "node.h"
#include "lit.h"

namespace chdl {
  template <unsigned M, unsigned N>
    bvec<N> Memory(bvec<N> d, bvec<M> a, node w, std::string filename = "")
  {
    // Provides interface to low-level node structures; prototyped here to avoid
    // pollution of chdl namespace.
    std::vector<node> memory_internal(
      std::vector<node> &d, std::vector<node> &a, node w, std::string filename
    );

    std::vector<node> dvec, avec, qvec;
    for (unsigned i = 0; i < M; ++i) avec.push_back(a[i]);
    for (unsigned i = 0; i < N; ++i) dvec.push_back(d[i]);
  
    qvec = memory_internal(dvec, avec, w, filename);

    bvec<N> q;
    for (unsigned i = 0; i < N; ++i) q[i] = qvec[i];
    
    return q;
  }

  template <unsigned M, unsigned N>
    bvec<N> Rom(bvec<M> a, std::string filename)
  {
    return Memory(Lit<N>(0), a, Lit(0), filename);
  }
};

#endif
