// This is the CHDL programmer facing side of the optimization layer. It is used
// solely for the purpose of invoking the optimization layer.
#ifndef __OPT_H
#define __OPT_H

#include "node.h"

namespace chdl {
  // These functions are all called by optimize()
  void opt_dead_node_elimination();
  void opt_contract();
  void opt_combine_literals();
  void opt_dedup();

  void optimize();

  // These are more situation-specic optimizations
  void opt_limit_fanout(unsigned max_fanout);
};

#endif
