// Tickable.
#ifndef CHDL_TICKABLE_H
#define CHDL_TICKABLE_H

#include "cdomain.h"

#include <vector>
#include <set>

namespace chdl {
  struct tickable;

  std::vector<std::vector<tickable*> > &tickables();

  class tickable {
  public:
    tickable() { tickables()[cur_clock_domain()].push_back(this); }
    ~tickable();

    virtual void tick() = 0;
    virtual void tock() = 0;
  };
};

#endif
