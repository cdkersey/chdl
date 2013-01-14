#ifndef __GATESIMPL_H
#define __GATESIMPL_H

#include "nodeimpl.h"

namespace chdl {
  class invimpl : public nodeimpl {
  public:
    invimpl(node in) { src.push_back(in); }

    virtual bool eval();
    virtual void print(std::ostream &out);
  };

  class nandimpl : public nodeimpl {
  public:
    nandimpl(node a, node b) { src.push_back(a); src.push_back(b); }

    virtual bool eval();
    virtual void print(std::ostream &out);
  };
};

#endif
