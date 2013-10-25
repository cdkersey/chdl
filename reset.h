#ifndef __RESET_H
#define __RESET_H

#include <vector>
#include <iostream>

namespace chdl {
  void reset(); // Completely reset CHDL state

  struct reset_func_base {
    virtual ~reset_func_base();
    virtual void operator()() = 0;
    static std::vector<reset_func_base*> *rfuns();
  };

  template<typename T> struct reset_func : public reset_func_base {
    reset_func(const T &f): f(f) { rfuns()->push_back(this); }
    virtual void operator()() { f(); }
    const T &f;
  };
}

#define CHDL_REGISTER_RESET(x) namespace { \
  static reset_func<decltype(x)> __chdl_reset__ ## x (x); \
}

#endif
