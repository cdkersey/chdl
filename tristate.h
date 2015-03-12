#ifndef __TRISTATE_H
#define __TRISTATE_H

#include "node.h"

namespace chdl {
  struct tristatenode : public node {
    tristatenode();
    tristatenode(nodeid_t id);
    void connect(node input, node enable);
  };
};

#endif
