#ifndef CHDL_CASSIGN_H
#define CHDL_CASSIGN_H

#include "bvec.h"

namespace chdl {
  template <unsigned N> struct cassign {
    cassign(const bvec<N> &v): v(v) {} 
    cassign<N> IF(node x, const bvec<N> &a) {
      bvec<N> bv;
      v = Mux(x, bv, a);

      return cassign(bv);
    }

    void ELSE(const bvec<N> &a) { v = a; }

    bvec<N> v;
  };

  struct node_cassign {
    node_cassign(const node &n): n(n) {}

    node_cassign IF(node x, const node &a) {
      node y;
      n = Mux(x, y, a);
      return node_cassign(y);
    }

    node_cassign ELSE(const node &a) { n = a; }

    node n;
  };

  template <unsigned N> cassign<N> Cassign(const bvec<N> &v);
  static node_cassign Cassign(const node &n);
};

template <unsigned N> chdl::cassign<N> chdl::Cassign(const chdl::bvec<N> &v) {
  return chdl::cassign<N>(v);
}

static chdl::node_cassign chdl::Cassign(const node &n) {
  return chdl::node_cassign(n);
}

#endif
