#include <iostream>

#include "netlist.h"

#include "node.h"
#include "nodeimpl.h"
#include "tap.h"

void chdl::print_netlist(std::ostream &out) {
  using namespace std;
  using namespace chdl;
  out << "outputs" << endl;
  print_tap_nodes(out);
  out << "design" << endl;
  for (nodeid_t i = 0; i < nodes.size(); ++i) nodes[i]->print(out);
}
