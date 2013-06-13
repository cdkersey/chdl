#include <iostream>
#include <fstream>

#include <chdl/chdl.h>

using namespace std;
using namespace chdl;

int main(int argc, char **argv) {
  bvec<8> x, y;
  x = Reg(x + Lit<8>(1));
  y = Zext<8>(x[range<0, 3>()]) * Zext<8>(x[range<4, 7>()]);
  TAP(x); TAP(y);

  // The simulation (generate .vcd file)
  optimize();

  ofstream wave_file("example.vcd");
  run(wave_file, 1000);

  ofstream c_file("example-c.c");
  print_c(c_file);
  c_file.close();

  return 0;
}
