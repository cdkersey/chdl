#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

#include <gateops.h>
#include <bvec-basic-op.h>
#include <adder.h>
#include <shifter.h>
#include <mux.h>
#include <enc.h>
#include <llmem.h>
#include <memory.h>

#include <tristate.h>
#include <input.h>

#include <opt.h>
#include <tap.h>
#include <sim.h>
#include <netlist.h>
#include <analysis.h>
#include <vis.h>

#include <cassign.h>
#include <console.h>

#include <hierarchy.h>

#include <cassign.h>

#include <techmap.h>

#include "report.h"

using namespace std;
using namespace chdl;

int main() {
  node v;
  bvec<8> c, r13c;
  ConsoleIn(v, c);
  ConsoleOut(v, r13c);

  node firstHalf = (c >= Lit<8>('a') && c < Lit<8>('a' + 13)) ||
                   (c >= Lit<8>('A') && c < Lit<8>('A' + 13));

  node secondHalf = (c >= Lit<8>('a' + 13) && c < Lit<8>('a' + 26)) ||
                    (c >= Lit<8>('A' + 13) && c < Lit<8>('A' + 26));

  Cassign(r13c).
    IF(firstHalf, c + Lit<8>(13)).
    IF(secondHalf, c - Lit<8>(13)).
    ELSE(c);

  TAP(v);
  TAP(c);
  TAP(r13c);

  optimize();

  ofstream vcd("example12.vcd");
  run(vcd, 1000);

  return 0;
}
