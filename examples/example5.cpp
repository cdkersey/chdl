// This example implements an LFSR
#include <iostream>
#include <fstream>

#include <gateops.h>
#include <bvec-basic.h>
#include <adder.h>
#include <mux.h>

#include <opt.h>
#include <tap.h>
#include <sim.h>
#include <netlist.h>
#include <vis.h>

#include <hierarchy.h>

using namespace std;
using namespace chdl;

node Lfsr16(const unsigned seed = 0x5eed) {
  bvec<16> sr;
  TAP(sr);
  for (unsigned i = 1; i < 16; ++i) sr[i] = Reg(sr[i-1], (seed>>i)&1);

  // This is a max-period LFSR for 16 bits
  bvec<3> taps;
  taps[0] = sr[0];
  taps[1] = sr[1];
  taps[2] = sr[15];

  TAP(taps);
  node next = XorN(taps);
  TAP(next);

  sr[0] = Reg(!next, seed&1);

  return next;
}

int main(int argc, char **argv) {
  node out = Lfsr16();
  TAP(out);

  // The simulation (generate .vcd file)
  optimize();

  ofstream wave_file("example5.vcd");
  run(wave_file, 1000);

  ofstream netlist_file("example5.nand");
  print_netlist(netlist_file);

  ofstream dot_file("example5.dot");
  //print_dot(dot_file);
  dot_schematic(dot_file);

  return 0;
}
