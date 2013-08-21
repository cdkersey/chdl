#ifndef __REG_H
#define __REG_H

#include <set>

#include "node.h"

namespace chdl {
  node Reg(node d, bool val=0);
  
  node Wreg(node w, node d);

  void get_reg_nodes(std::set<nodeid_t> &s);
  void get_reg_d_nodes(std::set<nodeid_t> &s);
};

#endif
