// This example implements an LFSR
#include <iostream>

#include <gateops.h>
#include <bvec-basic.h>
#include <adder.h>
#include <mux.h>

#include <opt.h>
#include <tap.h>
#include <sim.h>

using namespace std;
using namespace chdl;

template <unsigned N> node Lfsr() {
  rvec<N> sr(Reg<N>());
  TAP(sr);
  for (unsigned i = 1; i < N; ++i) sr[i].connect(sr[i-1]);

  // I have no idea if a fibbonaci LFSR has to do with the fibbonaci sequence,
  // but I'll pretend it does and use that to select my taps.
  bvec<4> taps;
  taps[0] = sr[N-1];
  taps[1] = sr[N-3];
  taps[2] = sr[N-4];
  taps[3] = sr[N-6];

  TAP(taps);
  node next = XorN(taps);
  TAP(next);

  sr[0].connect(!next);

  return next;
}

int main(int argc, char **argv) {
  node out = Lfsr<64>();
  TAP(out);

  // The simulation (generate .vcd file)
  optimize();
  run(cout, 65536);
}
