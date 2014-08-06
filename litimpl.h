#ifndef __LITIMPL_H
#define __LITIMPL_H

#include "nodeimpl.h"

namespace chdl {
  class litimpl : public nodeimpl {
  public:
    litimpl(bool v): val(v), undef(0) {}
    litimpl(): val(0), undef(1) {}

    virtual bool eval(evaluator_t &e);
    virtual void print(std::ostream &out);
    virtual void print_vl(std::ostream &out);

    bool is_undef() { return undef; }
    void resolve(bool x) { if (undef) val = x; else abort(); }

  private:
    bool val, undef;
  };
};

#endif
