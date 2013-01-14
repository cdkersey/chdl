#ifndef __REG_H
#define __REG_H

#include <set>

#include "node.h"

namespace chdl {
  struct reg : public node {
    reg() {}
    reg(nodeid_t id): node(id) {}

    void connect(node d);
  };

  reg Reg();

  void get_reg_nodes(std::set<nodeid_t> &s);
};

#endif
