#ifndef __REGIMPL_H
#define __REGIMPL_H

#include "nodeimpl.h"
#include "tickable.h"
#include "cdomain.h"

#include <iostream>

namespace chdl {
  struct regimpl : public tickable, public nodeimpl {
    regimpl(node d);
    ~regimpl();

    bool eval(cdomain_handle_t);
    void print(std::ostream &out);
    void print_vl(std::ostream &out);

    void tick(cdomain_handle_t cd) { next_q = nodes[d]->eval(cd); }
    void tock(cdomain_handle_t) { q = next_q; }

    node d;

    bool q, next_q;
  };
};

#endif
