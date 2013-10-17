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

#include <opt.h>
#include <tap.h>
#include <sim.h>
#include <netlist.h>
#include <analysis.h>
#include <vis.h>

#include <hierarchy.h>

#include <cassign.h>

#include "report.h"

using namespace std;
using namespace chdl;

int main() {
  bvec<2> counter;
  counter = Reg(counter + Lit<2>(1));

  bvec<4> counter_1h;

  TAP(counter); TAP(counter_1h);

  Cassign(counter_1h).
    IF(!counter[1]).
      IF(!counter[0]).THEN(Lit<4>(1)).
      ELSE(Lit<4>(2)).
    END().ELSE().
      IF(!counter[0]).THEN(Lit<4>(4)).
      IF(counter[0], Lit<4>(8)).
    END();
  
  optimize();

  ofstream vcdout("example10.vcd");
  run(vcdout, 20);
}
