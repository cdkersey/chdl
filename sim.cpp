#include <iostream>

#include "sim.h"
#include "tickable.h"
#include "tap.h"
#include "reset.h"

using namespace chdl;
using namespace std;

static cycle_t now = 0;
static void reset_now() { now = 0; }
CHDL_REGISTER_RESET(reset_now);

cycle_t chdl::sim_time() { return now; }

cycle_t chdl::advance(unsigned threads) {
  for (auto &t : tickables()[0]) t->tick();
  for (auto &t : tickables()[0]) t->tock();

  return now++;
}

void chdl::print_time(ostream &out) {
  out << '#' << now << endl;  
}

void chdl::run(ostream &vcdout, cycle_t time, unsigned threads) {
  print_vcd_header(vcdout);
  print_time(vcdout);
  for (unsigned i = now; i < time; ++i) {
    print_taps(vcdout);
    advance(threads);
    print_time(vcdout);
  }
}
