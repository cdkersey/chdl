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

#include <vis.h>
#include <hierarchy.h>

using namespace std;
using namespace chdl;

int main(int argc, char **argv) {
  // The design
  node r0, r1;

  // We are going to intentionally re-assign this node to show that it is
  // possible.
  node lit1;
  node lit2(lit1);

  r0 = Reg(Xor(r0, lit2));
  r1 = Reg(!Xor(r0, r1));

  lit1 = Lit(1); // This is retroactively effective.

  TAP(r0);
  TAP(r1);

  // The simulation (generate .vcd file)
  optimize();

  ofstream wave_file("example0.vcd");
  run(wave_file, 8);

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
