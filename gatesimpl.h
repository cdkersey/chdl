#ifndef __GATESIMPL_H
#define __GATESIMPL_H

#include "nodeimpl.h"
#include "sim.h"

namespace chdl {
  class invimpl : public nodeimpl {
  public:
    invimpl(node in) { src.push_back(in); }

    virtual bool eval(evaluator_t &e);
    virtual void print(std::ostream &out);
    virtual void print_vl(std::ostream &out);

    virtual void gen_eval(evaluator_t &e, execbuf &b, nodebuf_t &from);
  };

  class nandimpl : public nodeimpl {
  public:
    nandimpl(node a, node b) {
      src.push_back(a); src.push_back(b);
    }

    virtual bool eval(evaluator_t &e);
    virtual void print(std::ostream &out);
    virtual void print_vl(std::ostream &out);

    virtual void gen_eval(evaluator_t &e, execbuf &b, nodebuf_t &from);
  };
};

#endif
