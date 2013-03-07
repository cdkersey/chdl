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

using namespace std;
using namespace chdl;

int main(int argc, char **argv) {
  // The design
  bvec<3> a;
  a = Reg(a + Lit<3>(1));
  bvec<8> adec(Decoder(a, Lit(1)));

  vec<8, bvec<32>> muxin;
  muxin[0] = Lit<32>(0x01010101);
  muxin[1] = Lit<32>(0x20202020);
  muxin[2] = Lit<32>(0x03030303);
  muxin[3] = Lit<32>(0x40404040);
  muxin[4] = Lit<32>(0x05050505);
  muxin[5] = Lit<32>(0x60606060);
  muxin[6] = Lit<32>(0x07070707);
  muxin[7] = Lit<32>(0x80808080);

  bvec<32> b(Mux(a, muxin));

  TAP(a); TAP(b); TAP(adec);

  // The simulation (generate .vcd file)
  optimize();

  ofstream wave_file("example4.vcd");
  run(wave_file, 32);

  ofstream netlist_file("example4.nand");
  print_netlist(netlist_file);

  ofstream dot_file("example4.dot");
  print_dot(dot_file);
}
