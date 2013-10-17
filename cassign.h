#ifndef CHDL_CASSIGN_H
#define CHDL_CASSIGN_H

#include <stack>

#include "bvec.h"

namespace chdl {
  template <unsigned N> struct cassign {
    cassign(const bvec<N> &v): v(v) {}
    cassign<N> IF(node x, const bvec<N> &a) {
      bvec<N> bv;
      v = Mux(x, bv, a);

      return cassign(bv);
    }

    cassign<N> IF(node x) {
      bvec<N> nested_val;
      cassign<N> nest(nested_val);
      cstack.push(IF(x, nested_val));
      return nest;
    }

    cassign<N> ELSE() {
      bvec<N> nested_val;
      cassign<N> nest(nested_val);
      cstack.push(ELSE(nested_val));
      return nest;
    }

    cassign<N> END() {
      if (cstack.empty()) abort(); // assert(!cstack.empty());
      cassign<N> rval(cstack.top());
      cstack.pop();
      return rval;
    }

    cassign<N> ELSE(const bvec<N> &a) { v = a; return *this; }

    cassign<N> THEN() { return *this; }
    cassign<N> THEN(const bvec<N> &a) { return ELSE(a).END(); }

    bvec<N> v;

    static std::stack<cassign> cstack;
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

  template <unsigned N> std::stack<cassign<N>> cassign<N>::cstack;

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
