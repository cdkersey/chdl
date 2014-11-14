#include <fstream>

#include <gateops.h>
#include <bvec-basic-op.h>
#include <adder.h>
#include <shifter.h>
#include <mux.h>
#include <enc.h>
#include <llmem.h>
#include <memory.h>

#include <opt.h>
#include <tap.h>
#include <sim.h>
#include <netlist.h>
#include <analysis.h>
#include <vis.h>
#include <cassign.h>
#include <egress.h>

#include <hierarchy.h>
#include <techmap.h>

#include "report.h"

using namespace std;
using namespace chdl;

int main() {
  vec<2, vec<2, bvec<2> > > ocho;
  TAP(ocho);
  for (unsigned i = 0; i < 2; ++i)
    for (unsigned j = 0; j < 2; ++j)
      ocho[i][j] = Reg(ocho[i][j] + Lit<2>(i ^ j), i + j);

  bvec<8> ochoflat(Flatten(ocho));
  TAP(ochoflat);

  #if 0

  bvec<32> c1; c1 = Reg(c1 + Lit<32>(1));

  push_clock_domain(3);
  bvec<4> next_c3, c3(Reg(next_c3));
  next_c3 = c3 + Lit<4>(1);  

  pop_clock_domain();

  push_clock_domain(5);
  bvec<32> next_c5, c5(Reg(next_c5));
  Cassign(next_c5).
    IF(c3 == Lit<4>(3), c5 + Lit<32>(2)).
    ELSE(c5 + Lit<32>(1));
  pop_clock_domain();

  TAP(c1);
  TAP(c3);
  TAP(c5);

  #endif

  optimize();

   // Do the simulation
  ofstream wave_file("example6.vcd");
  run(wave_file, 1000);

  report();

  return 0;
}
