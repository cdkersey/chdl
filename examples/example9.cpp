#include <iostream>
#include <fstream>

#include <chdl.h>

using namespace std;
using namespace chdl;

template <unsigned N> bvec<N> Counter() {
  bvec<N> c;
  c = Reg(c + Lit<N>(1));
  return c;
}

int main(int argc, char **argv) {
  node l;
  bvec<8> ctr(Counter<8>()), lval(Latch(l, ctr));
  l = Mux(ctr[range<0,4>()], Lit<32>(0xdeadbee5));

  TAP(ctr); TAP(lval); TAP(l);

  optimize();

  ofstream nand_file("example9.nand");
  print_netlist(nand_file);

  ofstream wave_file("example9.vcd");
  run(wave_file, 256);

  return 0;
}
