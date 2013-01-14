// This is the CHDL programmer facing side of the optimization layer. It is used
// solely for the purpose of invoking the optimization layer.
#ifndef __OPT_H
#define __OPT_H

#include "node.h"

namespace chdl {
  void opt_dead_node_elimination();

  void optimize();
};

#endif
