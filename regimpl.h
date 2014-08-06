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

    bool eval(evaluator_t &e);
    void print(std::ostream &out);
    void print_vl(std::ostream &out);

    void tick(evaluator_t &e) { next_q = e(d); }
    void tock(evaluator_t &e) { q = next_q; }

    void gen_eval(cdomain_handle_t cd, execbuf &b, nodebuf_t &from);
    void gen_store_result(execbuf &b, nodebuf_t &from, nodebuf_t &to);

    node d;

    bool q, next_q;
  };
};

#endif
