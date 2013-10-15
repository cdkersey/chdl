#ifndef CHDL_CASSIGN_H
#define CHDL_CASSIGN_H

#include "bvec.h"

namespace chdl {
  template <unsigned N> struct cassign {
    cassign(bvec<N> &v): v(v) {} 
    cassign<N> IF(node x, bvec<N> &a) {
      bvec<N> bv;
      v = Mux(x, bv, a);

      return cassign(bv);
    }

    void ELSE(bvec<N> &a) { v = a; }

    bvec<N> v;
  };

  template <unsigned N> cassign<N> Cassign(bvec<N> &v);
};

template <unsigned N> chdl::cassign<N> chdl::Cassign(chdl::bvec<N> &v) {
  return chdl::cassign<N>(v);
}

#endif
