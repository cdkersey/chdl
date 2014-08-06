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

    bool eval(cdomain_handle_t cd);
    void print(std::ostream &out);
    void print_vl(std::ostream &out);

    void tick(cdomain_handle_t cd) { next_q = nodes[d]->eval(cd); }
    void tock(cdomain_handle_t cd) { q = next_q; }

    void gen_eval(cdomain_handle_t cd, execbuf &b, nodebuf_t &from);
    void gen_store_result(execbuf &b, nodebuf_t &from, nodebuf_t &to);

    node d;

    bool q, next_q;
  };
};

#endif
