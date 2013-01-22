#include <iostream>
#include <fstream>

#include <gates.h>
#include <gateops.h>
#include <lit.h>
#include <netlist.h>
#include <reg.h>

#include <tap.h>
#include <opt.h>
#include <sim.h>

using namespace std;
using namespace chdl;

int main(int argc, char **argv) {
  // The design
  reg r0(Reg()), r1(Reg());

  // We are going to intentionally re-assign this node to show that it is
  // possible.
  node lit1(Lit(0));

  r0.connect(Xor(r0, lit1));
  r1.connect(!Xor(r0, r1));

  lit1 = Lit(1); // This is retroactively effective.

  TAP(r0);
  TAP(r1);

  // The simulation (generate .vcd file)
  optimize();
  run(cout, 8);

  ofstream netlist_file("example0.nand");
  print_netlist(netlist_file);
  netlist_file.close();
}
