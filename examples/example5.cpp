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

template <unsigned N> node Lfsr() {
  bvec<N> sr;
  TAP(sr);
  for (unsigned i = 1; i < N; ++i) sr[i] = Reg(sr[i-1]);

  // I have no idea if a fibbonaci LFSR has to do with the fibbonaci sequence,
  // but I'll pretend it does and use that to select my taps.
  bvec<4> taps;
  taps[0] = sr[N-1];
  taps[1] = sr[N-3];
  taps[2] = sr[N-4];
  taps[3] = sr[N-6];

  TAP(taps);
  node next = XorN(taps);
  TAP(next);

  sr[0] = Reg(!next);

  return next;
}

int main(int argc, char **argv) {
  node out = Lfsr<64>();
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
