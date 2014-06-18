#include <iostream>
#include <functional>

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

void chdl::run(ostream &vcdout, function<bool()> end_condition,
               unsigned threads)
{
  using namespace std;
  using namespace chdl;

  vector<unsigned> &ti(tick_intervals());

  print_vcd_header(vcdout);
  print_time(vcdout);
  do {
    print_taps(vcdout);
    for (unsigned j = 0; j < ti.size(); ++j)
      if (sim_time()%ti[j] == 0) advance(threads, j);
    print_time(vcdout);
  } while (!end_condition());
}


void chdl::run(ostream &vcdout, cycle_t time, unsigned threads) {
  run(vcdout, [time](){ return now == time; }, threads);
}

void chdl::run(ostream &vcdout, bool &stop, unsigned threads) {
  run(vcdout, [&stop](){ return stop; }, threads);
}

void chdl::run(ostream &vcdout, bool &stop, cycle_t tmax, unsigned threads) {
  run(vcdout, [&stop, tmax](){ return now == tmax || stop; }, threads);
}
