#include <iostream>

#include <map>

#include "netlist.h"

#include "node.h"
#include "nodeimpl.h"
#include "tap.h"
#include "input.h"
#include "reg.h"

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
  // First, compute "logic layer" of each node, its distance from the farthest
  // register or literal. This is used to determine the order in which node
  // values are computed.
  map<nodeid_t, int> ll;
  multimap<int, nodeid_t> ll_r;
  set<nodeid_t> regs;
  get_reg_nodes(regs);
  int max_ll(0);
  for (auto r : regs) ll[r] = 0;
  bool changed;
  do {
    changed = false;

    for (nodeid_t i = 0; i < nodes.size(); ++i) {
      int new_ll(0);
      for (unsigned j = 0; j < nodes[i]->src.size(); ++j) {
        nodeid_t s(nodes[i]->src[j]);
        if (ll.find(s) != ll.end() && ll[s] >= new_ll)
          new_ll = ll[s] + 1;
      }
      if (ll.find(i) == ll.end() || new_ll > ll[i]) {
        changed = true; 
        ll[i] = new_ll;
      }
      if (new_ll > max_ll) max_ll = new_ll;
    }
  } while(changed);

  for (auto l : ll) ll_r.insert(pair<int, nodeid_t>(l.second, l.first));
  
  // Boilerplate top
  out << "#include <stdio.h>\n"
         "#include <stdlib.h>\n"
         "#include <string.h>\n"
         "#include <sys/time.h>\n";

  out << "unsigned long get_time_us() {\n"
         "  struct timeval tv;\n"
         "  gettimeofday(&tv, NULL);\n"
         "  return 1000000ul * tv.tv_sec + tv.tv_usec;\n"
         "}\n\n";

  out << "void sim_main(unsigned long stopat);\n\n"
      << "int main(int argc, char** argv) {\n"
      << "  sim_main(argc == 2?atol(argv[1]):1000);\n"
      << "  return 0;\n"
      << "}\n\n"
      << "void sim_main(unsigned long stopat) {\n";

  // Declarations
  regimpl::assign_rids();
  out << "  char regs0[" << num_regs() << "], regs1[" << num_regs() << "],"
         " *regs_from = regs0, *regs_to = regs1;\n\n"
         "  memset(regs0, 0, " << num_regs() << ");\n"
         "  memset(regs1, 0, " << num_regs() << ");\n\n";

  for (nodeid_t i = 0; i < nodes.size(); ++i) nodes[i]->print_c_decl(out);

  out << "\n  unsigned long start_time = get_time_us();\n\n";

  // Code to print VCD header
  print_taps_c_head(out);

  // Loop top
  out << "  unsigned long i;\n"
      << "  for (i = 0; i < stopat; i++) {\n";

  // Register transfer functions.
  for (auto p : ll_r) {
    nodeid_t id(p.second);

    if (!dynamic_cast<regimpl*>(nodes[id]))
      nodes[id]->print_c_impl(out);
  }

  // Print outputs
  print_taps_c_body(out);

  // Final operations (used for memory writes)
  for (auto p : ll_r) {
    nodeid_t id(p.second);

    nodes[id]->print_c_final (out);
  }

  out << "    // Register updates.\n";
  for (auto r : regs)
    if (dynamic_cast<regimpl*>(nodes[r]))
      nodes[r]->print_c_impl(out);

  // Flip register states.
  out << "    { char *t = regs_from; regs_from = regs_to; regs_to = t; }\n";

  // Loop bottom
  out << "  }\n";

  out << "\n  unsigned long end_time = get_time_us();\n"
         "  fprintf(stderr, \"%fms.\\n\", (end_time-start_time)/1000.0);\n\n";

  // Boilerplate bottom
  out << "}\n";
}
