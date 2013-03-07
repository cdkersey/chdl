#include <iostream>
#include <fstream>

#include <netlist.h>

#include <gateops.h>
#include <bvec-basic-op.h>

#include <adder.h>
#include <shifter.h>

#include <opt.h>
#include <tap.h>
#include <sim.h>
#include <vis.h>

using namespace std;
using namespace chdl;

int main(int argc, char **argv) {
#if 0
  // If you predefine your d and give it as an argument to Reg, you do not need
  // to use rvecs (or connect)
  bvec<4> rplus1, r(Reg<4>(rplus1));
  bvec<8> t;
  bvec<16> x(Lit<16>(1) << r);
  rplus1 = Adder(r, Lit<4>(1));

  t[range<0,3>()] = bvec<4>(r[range<0,3>()]);
  t[range<4,7>()] = r[range<0,3>()];

  cerr << "t[0] = " << t[0] << endl;

  TAP(r); TAP(t); TAP(x);
#else
  bvec<8> x;
  x = Reg(x + Lit<8>(1));
  TAP(x);
#endif

  // The simulation (generate .vcd file)
  optimize();

  ofstream wave_file("example1.vcd");
  run(wave_file, 8);

  ofstream netlist_file("example1.nand");
  print_netlist(netlist_file);
  netlist_file.close();

  ofstream dot_file("example1.dot");
  print_dot(dot_file);
  dot_file.close();
}
