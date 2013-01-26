#include <iostream>
#include <fstream>

#include <gateops.h>
#include <bvec-basic-op.h>
#include <netlist.h>

#include <adder.h>
#include <mult.h>

#include <tap.h>
#include <sim.h>
#include <opt.h>

using namespace std;
using namespace chdl;

int main(int argc, char **argv) {
  // The design
  rvec<4> a(Reg<4>()), b(Reg<4>()), c(Reg<4>()), d(Reg<4>());
  bvec<8> v, v2;
  bvec<4> w, x, y, z;
  bvec<16> o;

  a.connect(a + Lit<4>(1));
  b.connect(b + Lit<4>(3));
  c.connect(c + Lit<4>(5));
  d.connect(d + Lit<4>(7));

  o[range< 0, 3>()] = a;
  o[range< 4, 7>()] = b;
  o[range< 8,11>()] = c;
  o[range<12,15>()] = d;

  v = Cat(Lit<4>(0), a) * Cat(Lit<4>(0), b);
  w = a & b & c & d;
  x = a ^ b;
  y = a | b | c | d;
  z = a + b;

  node a_b_eq(a == b);

  TAP(a); TAP(b); TAP(c); TAP(d);
  TAP(v); TAP(w); TAP(x); TAP(y); TAP(z);
  TAP(o);
  TAP(a_b_eq);

  // The simulation (generate .vcd file)
  optimize();
  run(cout, 10);

  ofstream netlist_file("example2.nand");
  print_netlist(netlist_file);
  netlist_file.close();
}
