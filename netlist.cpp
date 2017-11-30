#include <iostream>

#include "netlist.h"

#include "node.h"
#include "nodeimpl.h"
#include "tap.h"
#include "input.h"
#include "regimpl.h"
#include "cdomain.h"

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

void chdl::print_verilog(
  const char* module_name, ostream &out, bool reset_signal
) {
  out << "module " << module_name << '(' << endl << "  phi";

  for (unsigned cd = 1; cd < clock_domains(); ++cd)
    out << ", phi" << cd;

  if (reset_signal) {
    out << ", reset";
  }

  print_inputs_vl_head(out);
  print_taps_vl_head(out);
  out << endl << ");" << endl << endl << "  input phi;" << endl;

  for (unsigned cd = 1; cd < clock_domains(); ++cd)
    out << "input phi" << cd << ';' << endl;

  if (reset_signal) {
    out << "  input reset;" << endl;
  }
  print_inputs_vl_body(out);

  if (!reset_signal) {
    out << "  wire reset;" << endl;
    out << "  assign reset = 0;" << endl;
  }

  set<nodeid_t> regs;
  get_reg_nodes(regs);
  for (nodeid_t i = 0; i < nodes.size(); ++i)
    if (regimpl *r = dynamic_cast<regimpl*>(nodes[i]))
      out << "  reg __x" << i << ';' << endl;
    else
      out << "  wire __x" << i << ';' << endl;

  for (nodeid_t i = 0; i < nodes.size(); ++i) nodes[i]->print_vl(out);

  print_taps_vl_body(out);

  out << "endmodule" << endl;
}
