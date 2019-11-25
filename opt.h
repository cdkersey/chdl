// This is the CHDL programmer facing side of the optimization layer. It is used
// solely for the purpose of invoking the optimization layer.
#ifndef __OPT_H
#define __OPT_H

#include "node.h"

#include <vector>

namespace chdl {
  // Lightweight dead node elimination for use by library code. Keep the dead
  // nodes in check.
  void node_sweep();

  // These functions are all called by optimize()
  void opt_dead_node_elimination();
  void opt_contract();
  void opt_combine_literals();
  void opt_dedup();
  void opt_tristate_merge();
  void opt_assoc_balance();
  void opt_set_dontcare();
  void opt_reg_retime(int iters = 100);
  
  void optimize();

  // More situation-specic optimizations
  void opt_limit_fanout(unsigned max_fanout);

  // Meta-optimizations; building blocks for domain-specific optimizations
  void order(std::vector<nodeid_t> &o, unsigned steps=0);
};

#endif
