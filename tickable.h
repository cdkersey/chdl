// Tickable.
#ifndef __TICKABLE_H
#define __TICKABLE_H

#include <vector>

namespace chdl {
  struct tickable;

  extern std::vector<tickable*> tickables;

  class tickable {
  public:
    tickable() { tickables.push_back(this); }
    ~tickable();

    virtual void tick() = 0;
    virtual void tock() = 0;
  };
};

#endif
