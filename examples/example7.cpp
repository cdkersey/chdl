// This example is a non-simulable module with inputs.
#include <iostream>
#include <sstream>
#include <fstream>

#include <gateops.h>
#include <bvec-basic-op.h>
#include <adder.h>
#include <mult.h>
#include <mux.h>
#include <memory.h>
#include <input.h>

#include <opt.h>
#include <tap.h>
#include <sim.h>
#include <netlist.h>

using namespace std;
using namespace chdl;

int main(int argc, char **argv) {
  // The design: multiply two 16-bit input numbers
  bvec<2> a = Input<2>("a");
  bvec<2> b = Input<2>("b");

  bvec<4> out = Cat(Lit<2>(0), a) * Cat(Lit<2>(0), b);
  TAP(out);

  // Emit a netlist
  optimize();
  ofstream netlist_file("example7.nand");
  print_netlist(netlist_file);
  netlist_file.close();
}
