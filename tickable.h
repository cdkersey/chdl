// Tickable.
#ifndef CHDL_TICKABLE_H
#define CHDL_TICKABLE_H

#include "cdomain.h"
#include "nodeimpl.h"

#include <vector>
#include <set>

namespace chdl {
  class tickable;

  std::vector<std::vector<tickable*> > &tickables();

  class tickable {
  public:
    tickable() { tickables()[cur_clock_domain()].push_back(this); }
    ~tickable();

    virtual void pre_tick(evaluator_t &e) {}
    virtual void tick(evaluator_t &e) {}
    virtual void tock(evaluator_t &e) {}
    virtual void post_tock(evaluator_t &e) {}

    cdomain_handle_t cd;
  };
};

#endif
