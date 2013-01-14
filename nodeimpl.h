// Node implementation.
#ifndef __NODEIMPL_H
#define __NODEIMPL_H

#include <vector>
#include <iostream>

#include "node.h"

namespace chdl {
  struct nodeimpl;

  extern std::vector<nodeimpl*> nodes;

  struct nodeimpl {
    nodeimpl() { id = nodes.size(); nodes.push_back(this); }
    virtual ~nodeimpl() {}

    virtual bool eval() = 0;
    virtual void print(std::ostream &) = 0;

    // The node keeps a copy of its ID. This makes mapping from nodeimpl
    // pointers to node objects straightforward.
    nodeid_t id;

    std::vector<node> src;
  };
};

#endif
