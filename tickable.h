// Tickable.
#ifndef CHDL_TICKABLE_H
#define CHDL_TICKABLE_H

#include "cdomain.h"

#include <vector>
#include <set>

namespace chdl {
  class tickable;

  std::vector<std::vector<tickable*> > &tickables();

  class tickable {
  public:
    tickable() : cd(cur_clock_domain())
      { tickables()[cur_clock_domain()].push_back(this); }
    ~tickable();

    virtual void pre_tick(cdomain_handle_t) {}
    virtual void tick(cdomain_handle_t) {}
    virtual void tock(cdomain_handle_t) {}
    virtual void post_tock(cdomain_handle_t) {}

    cdomain_handle_t cd;
  };
};

#endif
