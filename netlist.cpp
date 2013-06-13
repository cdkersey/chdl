#include <iostream>

#include "netlist.h"

#include "node.h"
#include "nodeimpl.h"
#include "tap.h"
#include "input.h"

#include "regimpl.h"
#include "analysis.h"

using namespace std;
using namespace chdl;

void chdl::print_netlist(ostream &out) {
  out << "inputs" << endl;
  print_input_nodes(out);
  out << "outputs" << endl;
  print_tap_nodes(out);
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

  for (nodeid_t i = 0; i < nodes.size(); ++i) nodes[i]->print_vl(out);

  out << "endmodule" << endl;
}

void chdl::print_c(ostream &out) {
  // Boilerplate top
  out << "#include <stdio.h>\n"
         "#include <string.h>\n\n"
         "int main(int argc, char** argv) {\n";

  // Declarations
  regimpl::assign_rids();
  out << "  char regs0[" << num_regs() << "], regs1[" << num_regs() << "],"
         " *regs_from = regs0, *regs_to = regs1;\n\n"
         "  bzero(regs0, " << num_regs() << ");\n"
         "  bzero(regs1, " << num_regs() << ");\n\n";

  for (nodeid_t i = 0; i < nodes.size(); ++i) nodes[i]->print_c_decl(out);

  // Loop top
  out << "  unsigned long stopat = (argc == 2)?atol(argv[1]):1000, i;\n"
      << "  for (i = 0; i < stopat; i++) {\n";

  // Print state of taps
  print_taps_c(out);

  // Register transfer functions
  for (nodeid_t i = 0; i < nodes.size(); ++i) nodes[i]->print_c_impl(out);

  // Flip register states.
  out << "    { char *t = regs_from; regs_from = regs_to; regs_to = t; }\n";

  // Loop bottom
  out << "  }\n";

  // Boilerplate bottom
  out << "  return 0;\n"
         "}\n";
}
