#include <iostream>

#include "sim.h"
#include "tickable.h"
#include "tap.h"
#include "reset.h"
#include "cdomain.h"

using namespace chdl;
using namespace std;

static cycle_t now = 0;
static void reset_now() { now = 0; }
CHDL_REGISTER_RESET(reset_now);

cycle_t chdl::sim_time() { return now; }

cycle_t chdl::advance(unsigned threads, cdomain_handle_t cd) {
  for (auto &t : tickables()[cd]) t->pre_tick();
  for (auto &t : tickables()[cd]) t->tick();
  for (auto &t : tickables()[cd]) t->tock();
  for (auto &t : tickables()[cd]) t->post_tock();

  if (cd == 0) now++;

  return now;
}

void chdl::print_time(ostream &out) {
  out << '#' << now << endl;  
}

void chdl::run(ostream &vcdout, cycle_t time, unsigned threads) {
  std::vector<unsigned> &ti(tick_intervals());

  print_vcd_header(vcdout);
  print_time(vcdout);
  for (unsigned i = now; i < time; ++i) {
    print_taps(vcdout);
    for (unsigned j = 0; j < ti.size(); ++j)
      if (i%ti[j] == 0) advance(threads, j);
    print_time(vcdout);
  }
}
