#ifndef CHDL_CASSIGN_H
#define CHDL_CASSIGN_H

#include <sstream>
#include <stack>

#include "bvec.h"

#define GCC_CASSIGN_INSTRUMENTATION

namespace chdl {
  template <unsigned N> struct cassign {
    cassign(const bvec<N> &v): v(v) {}

    cassign<N> IF(node x, const bvec<N> &a
      #ifdef GCC_CASSIGN_INSTRUMENTATION
                  ,const char* func = __builtin_FUNCTION(),
                  unsigned line = __builtin_LINE()
      #endif
    ) {
      #ifdef GCC_CASSIGN_INSTRUMENTATION
      std::ostringstream hstr;
      hstr << "IF," << func << ':' << line;
      hierarchy_enter(hstr.str());
      #else
      HIERARCHY_ENTER();
      #endif
      bvec<N> bv;
      v = Mux(x, bv, a);
      hierarchy_exit();

      return cassign(bv);
    }

    cassign<N> IF(node x
      #ifdef GCC_CASSIGN_INSTRUMENTATION
                  ,const char* func = __builtin_FUNCTION(),
                  unsigned line = __builtin_LINE()
      #endif
    ) {
      #ifdef GCC_CASSIGN_INSTRUMENTATION
      std::ostringstream hstr;
      hstr << "IF," << func << ':' << line;
      hierarchy_enter(hstr.str());
      #else
      HIERARCHY_ENTER();
      #endif

      bvec<N> nested_val;
      cassign<N> nest(nested_val);
      cstack.push(IF(x, nested_val));
      hierarchy_exit();

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

  template <unsigned N> std::stack<cassign<N>> cassign<N>::cstack;

  template <unsigned N> cassign<N> Cassign(const bvec<N> &v);
  static cassign<1> Cassign(const node &n);
};

template <unsigned N> chdl::cassign<N> chdl::Cassign(const chdl::bvec<N> &v) {
  return chdl::cassign<N>(v);
}

static chdl::cassign<1> chdl::Cassign(const node &n) {
  return chdl::cassign<1>(bvec<1>(n));
}

#endif
