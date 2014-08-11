#include <iostream>
#include <fstream>

#include "../adder.h"
#include "../analysis.h"
#include "../assert.h"
#include "../bus.h"
#include "../bvec-basic.h"
#include "../bvec-basic-op.h"
#include "../bvec.h"
#include "../cassign.h"
#include "../cdomain.h"
#include "../chdl.h"
#include "../divider.h"
#include "../egress.h"
#include "../enc.h"
#include "../execbuf.h"
#include "../gateops.h"
#include "../gates.h"
#include "../gatesimpl.h"
#include "../hierarchy.h"
#include "../ingress.h"
#include "../input.h"
#include "../latch.h"
#include "../lit.h"
#include "../litimpl.h"
#include "../llmem.h"
#include "../memory.h"
#include "../mult.h"
#include "../mux.h"
#include "../netlist.h"
#include "../node.h"
#include "../nodeimpl.h"
#include "../opt.h"
#include "../reg.h"
#include "../regimpl.h"
#include "../reset.h"
#include "../shifter.h"
#include "../sim.h"
#include "../statemachine.h"
#include "../submodule.h"
#include "../tap.h"
#include "../techmap.h"
#include "../tickable.h"
#include "../trisimpl.h"
#include "../tristate.h"
#include "../vis.h"

using namespace std;
using namespace chdl;

int main(int argc, char **argv) {
  push_clock_domain(3);
  bvec<4> ctr3; ctr3 = Reg(ctr3 + Lit<4>(1));
  pop_clock_domain();

  push_clock_domain(5);
  bvec<4> ctr5; ctr5 = Reg(ctr5 + Lit<4>(1));
  pop_clock_domain();

  node eq(ctr3 == ctr5);
  TAP(ctr3);
  TAP(ctr5);
  TAP(eq);

  // The simulation (generate .vcd file)
  optimize();

  ofstream wave_file("example0.vcd");
  run_trans(wave_file, 1000);

  ofstream netlist_file("example0.nand");
  print_netlist(netlist_file);
  netlist_file.close();

  ofstream dot_file("example0.dot");
  print_dot(dot_file);
  dot_file.close();

  ofstream h_file("example0.hier");
  print_hierarchy(h_file);

  return 0;
}
