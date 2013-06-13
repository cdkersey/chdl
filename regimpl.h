#ifndef __REGIMPL_H
#define __REGIMPL_H

#include "nodeimpl.h"
#include "tickable.h"

#include <iostream>

namespace chdl {
  struct regimpl : public tickable, public nodeimpl {
    regimpl(node d);
    ~regimpl();

    bool eval();
    void print(std::ostream &out);
    void print_vl(std::ostream &out);

    void print_c_decl(std::ostream &out);
    void print_c_impl(std::ostream &out);
    void print_c_val(std::ostream &out);

    void tick() { next_q = nodes[d]->eval(); }
    void tock() { q = next_q; }

    node d;

    bool q, next_q;

    nodeid_t rid;
    static void assign_rids();
  };
};

#endif
