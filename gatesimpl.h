#ifndef __GATESIMPL_H
#define __GATESIMPL_H

#include "nodeimpl.h"
#include "sim.h"

namespace chdl {
  class invimpl : public nodeimpl {
  public:
    invimpl(node in): t_cval(~0ull) { src.push_back(in); }

    virtual bool eval();
    virtual void print(std::ostream &out);
    virtual void print_vl(std::ostream &out);

    virtual void print_c_val(std::ostream &out);

  private:
    bool cval;
    cycle_t t_cval;
  };

  class nandimpl : public nodeimpl {
  public:
    nandimpl(node a, node b): t_cval(~0ull) {
      src.push_back(a); src.push_back(b);
    }

    virtual bool eval();
    virtual void print(std::ostream &out);
    virtual void print_vl(std::ostream &out);

    virtual void print_c_val(std::ostream &out);

  private:
    bool cval;
    cycle_t t_cval;
  };
};

#endif
