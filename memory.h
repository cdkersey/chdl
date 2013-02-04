#ifndef __MEMORY_H
#define __MEMORY_H

#include <string>
#include <vector>

#include "bvec-basic.h"
#include "node.h"
#include "lit.h"

namespace chdl {
  template <unsigned M, unsigned N>
    bvec<N> Memory(
      bvec<M> qa, bvec<N> d, bvec<M> da, node w,
      std::string filename = "", bool sync=false
    )
  {
    // Provides interface to low-level node structures; prototyped here to avoid
    // pollution of chdl namespace.
    std::vector<node> memory_internal(
      std::vector<node> &qa, std::vector<node> &d, std::vector<node> &da,
      node w, std::string filename, bool sync
    );

    std::vector<node> qavec, dvec, davec, qvec;
    for (unsigned i = 0; i < M; ++i) {
      qavec.push_back(qa[i]);
      davec.push_back(da[i]);
    }
    for (unsigned i = 0; i < N; ++i)
      dvec.push_back(d[i]);
  
    qvec = memory_internal(qavec, dvec, davec, w, filename, sync);

    bvec<N> q;
    for (unsigned i = 0; i < N; ++i) q[i] = qvec[i];
    
    return q;
  }

  template <unsigned M, unsigned N>
    bvec<N> Memory(
      bvec<M> a, bvec<N> d, node w, std::string filename = "", bool sync = false
    )
  {
    return Memory(a, d, a, w, filename, sync);
  }

  template <unsigned M, unsigned N>
    bvec<N> Syncmem(
      bvec<M> qa, bvec <N> d, bvec<M> da, node w, std::string filename = ""
    )
  {
    return Memory(qa, d, da, w, filename, true);
  }

  template <unsigned M, unsigned N>
    bvec<N> Syncmem(
      bvec<M> a, bvec<N> d, node w, std::string filename = ""
    )
  {
    return Syncmem(a, d, a, w, filename);
  }

  template <unsigned M, unsigned N>
    bvec<N> Rom(bvec<M> qa, std::string filename)
  {
    return Memory(qa, Lit<N>(0), Lit<M>(0), Lit(0), filename);
  }

  template <unsigned M, unsigned N>
    bvec<N> Syncrom(bvec<M> qa, std::string filename)
  {
    return Syncmem(qa, Lit<M>(0), Lit<N>(0), Lit(0), filename);
  }
};

#endif
