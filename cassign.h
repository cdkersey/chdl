#ifndef CHDL_CASSIGN_H
#define CHDL_CASSIGN_H

#include <sstream>
#include <stack>

// #define GCC_CASSIGN_INSTRUMENTATION

namespace chdl {
  template <typename T> struct cassign {
    cassign(const T &v): v(v) {}

    cassign<T> IF(node x, const T &a
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
      T val;
      v = Mux(x, val, a);
      hierarchy_exit();

      return cassign(val);
    }

    cassign<T> IF(node x
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

      T nested_val;
      cassign<T> nest(nested_val);
      cstack.push(IF(x, nested_val));
      hierarchy_exit();

      return nest;
    }

    cassign<T> ELSE() {
      T nested_val;
      cassign<T> nest(nested_val);
      cstack.push(ELSE(nested_val));
      return nest;
    }

    cassign<T> END() {
      if (cstack.empty()) abort(); // assert(!cstack.empty());
      cassign<T> rval(cstack.top());
      cstack.pop();
      return rval;
    }

    cassign<T> ELSE(const T &a) { v = a; return *this; }

    cassign<T> THEN() { return *this; }
    cassign<T> THEN(const T &a) { return ELSE(a).END(); }

    T v;

    static std::stack<cassign> cstack;
  };

  template <typename T> std::stack<cassign<T>> cassign<T>::cstack;

  template <typename T> cassign<T> Cassign(const T &v);
};

template <typename T> chdl::cassign<T> chdl::Cassign(const T &v) {
  return chdl::cassign<T>(v);
}
#endif
