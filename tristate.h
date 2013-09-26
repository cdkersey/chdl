#ifndef __TRISTATE_H
#define __TRISTATE_H

#include "node.h"

namespace chdl {
  struct tristatenode : public node {
    tristatenode();
    void connect(node input, node enable);
    operator node() { return node(idx); }
  };
};

#endif
