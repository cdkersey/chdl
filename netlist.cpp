#include "netlist.h"

#include "node.h"
#include "nodeimpl.h"

void chdl::print_netlist(std::ostream &out) {
  for (nodeid_t i = 0; i < nodes.size(); ++i) nodes[i]->print(out);
}
