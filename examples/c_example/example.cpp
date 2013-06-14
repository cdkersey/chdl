#include <iostream>
#include <fstream>

#include <chdl/chdl.h>

using namespace std;
using namespace chdl;

int main(int argc, char **argv) {
  bvec<5> ctr; ctr = Reg(ctr + Lit<5>(1));
  bvec<4> adr(ctr[range<0,3>()]);
  bvec<8> wval;
  wval = Reg(wval + Lit<8>(17), 29);
  node w(!ctr[4]);

  bvec<8> q = Memory(adr, wval, w);

  TAP(wval); TAP(w); TAP(adr); TAP(q);

  // The simulation (generate .vcd file)
  optimize();

  ofstream wave_file("example.vcd");
  run(wave_file, 1000);

  ofstream c_file("example-c.c");
  print_c(c_file);
  c_file.close();

  ofstream nand_file("example.nand");
  print_netlist(nand_file);
  nand_file.close();

  return 0;
}
