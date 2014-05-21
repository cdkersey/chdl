#include <iostream>

#include "netlist.h"

#include "node.h"
#include "nodeimpl.h"
#include "tap.h"
#include "input.h"
#include "regimpl.h"

using namespace std;
using namespace chdl;

void chdl::print_netlist(ostream &out) {
  out << "inputs" << endl;
  print_input_nodes(out);
  out << "outputs" << endl;
  print_tap_nodes(out);
  out << "inout" << endl;
  print_io_tap_nodes(out);
  out << "design" << endl;
  for (nodeid_t i = 0; i < nodes.size(); ++i) nodes[i]->print(out);
}

void chdl::print_verilog(const char* module_name, ostream &out) {
  out << "module " << module_name << '(' << endl << "  phi";
  print_inputs_vl_head(out);
  print_taps_vl_head(out);
  out << endl << ");" << endl << endl << "  input phi;" << endl;
  print_inputs_vl_body(out);
  print_taps_vl_body(out);

  set<nodeid_t> regs;
  get_reg_nodes(regs);
  for (nodeid_t i = 0; i < nodes.size(); ++i)
    if (regimpl *r = dynamic_cast<regimpl*>(nodes[i]))
      out << "  reg __x" << i << ';' << endl;
    else
      out << "  wire __x" << i << ';' << endl;

  for (nodeid_t i = 0; i < nodes.size(); ++i) nodes[i]->print_vl(out);

  out << "endmodule" << endl;
}
