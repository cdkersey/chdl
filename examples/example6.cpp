#include <fstream>

#include "../adder.h"
#include "../analysis.h"
#include "../assert.h"
#include "../bus.h"
#include "../bvec-basic.h"
#include "../bvec-basic-op.h"
#include "../bvec.h"
#include "../cassign.h"
#include "../cdomain.h"
#include "../chdl.h"
#include "../divider.h"
#include "../egress.h"
#include "../enc.h"
#include "../execbuf.h"
#include "../gateops.h"
#include "../gates.h"
#include "../gatesimpl.h"
#include "../hierarchy.h"
#include "../ingress.h"
#include "../input.h"
#include "../latch.h"
#include "../lit.h"
#include "../litimpl.h"
#include "../llmem.h"
#include "../memory.h"
#include "../mult.h"
#include "../mux.h"
#include "../netlist.h"
#include "../node.h"
#include "../nodeimpl.h"
#include "../opt.h"
#include "../reg.h"
#include "../regimpl.h"
#include "../reset.h"
#include "../shifter.h"
#include "../sim.h"
#include "../statemachine.h"
#include "../submodule.h"
#include "../tap.h"
#include "../techmap.h"
#include "../tickable.h"
#include "../trisimpl.h"
#include "../tristate.h"
#include "../vis.h"

using namespace std;
using namespace chdl;

int main() {
  bvec<32> x, y, z, w;

  x = Reg(x + Lit<32>(13), 1234);
  y = Reg(x + Lit<32>(7), 1357);
  z = x * y;
  w = z / y;

  TAP(x); TAP(y); TAP(z); TAP(w);
  // TAP(y);

  optimize();

  ofstream vcd("example6.vcd");
  run_trans(vcd, 10000);

  return 0;
}
