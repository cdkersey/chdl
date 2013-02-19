#include <iostream>
#include <fstream>

#include <gateops.h>
#include <bvec-basic-op.h>
#include <netlist.h>

#include <adder.h>
#include <divider.h>
#include <mult.h>

#include <tap.h>
#include <sim.h>
#include <opt.h>

using namespace std;
using namespace chdl;

int main(int argc, char **argv) {
  // The design
  bvec<4> a, b, c, d, ax;
  bvec<8> v, v2;
  bvec<4> w, x, y, z;
  bvec<16> o;

  a = Reg(a + Lit<4>(1));
  b = Reg(b + Lit<4>(3));
  c = Reg(c + Lit<4>(5));
  d = Reg(d + Lit<4>(7));

  o[range< 0, 3>()] = a;
  o[range< 4, 7>()] = b;
  o[range< 8,11>()] = c;
  o[range<12,15>()] = d;


  v = Zext<8>(a) * Zext<8>(b);
  w = a & b & c & d;
  x = a ^ b;
  y = a | b | c | d;
  z = a + b;
  ax = Zext<4>(divider(v, Zext<8>(b)));

  node a_b_eq(a == b);

  TAP(a); TAP(b); TAP(c); TAP(d);
  TAP(ax);
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
