 #include <iostream>

#include <gateops.h>
#include <bvec-basic.h>
#include <adder.h>
#include <memory.h>

#include <opt.h>
#include <tap.h>
#include <sim.h>

using namespace std;
using namespace chdl;

int main(int argc, char **argv) {
  // The design
  rvec<5> a(Reg<5>());
  bvec<32> q, d(Lit<32>(0));

  q = Memory(d, bvec<5>(a), Lit(0), "sample.hex");

  a.connect(a + Lit<5>(1));

  TAP(a); TAP(d); TAP(q);

  // The simulation (generate .vcd file)
  optimize();
  run(cout, 32);
}
