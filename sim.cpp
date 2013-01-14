#include <iostream>

#include "sim.h"
#include "tickable.h"
#include "tap.h"

using namespace chdl;
using namespace std;

static cycle_t now = 0;

cycle_t chdl::advance() {
  for (size_t i = 0; i < tickables.size(); ++i) tickables[i]->tick();
  for (size_t i = 0; i < tickables.size(); ++i) tickables[i]->tock();

  ++now;
}

void chdl::print_time(ostream &out) {
  cout << '#' << now << endl;  
}

void chdl::run(ostream &vcdout, cycle_t time) {
  print_vcd_header(vcdout);
  print_time(vcdout);
  for (unsigned i = now; i < time; ++i) {
    print_taps(cout);
    advance();
    print_time(cout);
  }
}
