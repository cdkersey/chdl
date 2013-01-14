#ifndef __LITIMPL_H
#define __LITIMPL_H

#include "nodeimpl.h"

namespace chdl {
  class litimpl : public nodeimpl {
  public:
    litimpl(bool v): val(v) {}

    virtual bool eval();
    virtual void print(std::ostream &out);
  private:
    bool val;
  };
};

#endif
