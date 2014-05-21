#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

#include <gateops.h>
#include <bvec-basic-op.h>
#include <adder.h>
#include <shifter.h>
#include <mux.h>
#include <enc.h>
#include <llmem.h>
#include <memory.h>

#include <tristate.h>
#include <input.h>

#include <opt.h>
#include <tap.h>
#include <sim.h>
#include <netlist.h>
#include <analysis.h>
#include <vis.h>

#include <hierarchy.h>

#include <cassign.h>

#include <techmap.h>

#include "report.h"

using namespace std;
using namespace chdl;

int main() {
  bvec<4> counter;
  counter = Reg(counter + Lit<4>(1));

  tristatenode io;

  io.connect(counter[0], counter[range<1, 3>()] == Lit<3>(0));
  io.connect(!counter[0], counter[range<1, 3>()] == Lit<3>(1));

  OUTPUT(io);

  optimize();

  ofstream nandfile("example11.nand");
  print_netlist(nandfile);

  ofstream netlist("example11.netl");
  techmap(netlist);

  return 0;
}
