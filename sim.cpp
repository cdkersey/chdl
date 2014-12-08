#include <iostream>
#include <functional>
#include <vector>

#include "sim.h"
#include "tickable.h"
#include "tap.h"
#include "reset.h"
#include "cdomain.h"

using namespace chdl;
using namespace std;

vector<cycle_t> chdl::now{0};
static void reset_now() { now = vector<cycle_t>(1); }
CHDL_REGISTER_RESET(reset_now);

cycle_t chdl::sim_time(cdomain_handle_t cd) { return now[cd]; }

cycle_t chdl::advance(unsigned /* threads */, cdomain_handle_t cd) {
  for (auto &t : tickables()[cd]) t->pre_tick(cd);
  for (auto &t : tickables()[cd]) t->tick(cd);
  for (auto &t : tickables()[cd]) t->tock(cd);
  for (auto &t : tickables()[cd]) t->post_tock(cd);

  now[cd]++;

  return now[cd];
}

void chdl::print_time(ostream &out) {
  out << '#' << now[0] << endl;  
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

  call_final_funcs();
}


void chdl::run(ostream &vcdout, cycle_t time, unsigned threads) {
  run(vcdout, [time](){ return now[0] == time; }, threads);
}

void chdl::run(ostream &vcdout, bool &stop, unsigned threads) {
  run(vcdout, [&stop](){ return stop; }, threads);
}

void chdl::run(ostream &vcdout, bool &stop, cycle_t tmax, unsigned threads) {
  run(vcdout, [&stop, tmax](){ return now[0] == tmax || stop; }, threads);
}

vector<function<void()> > &final_funcs() {
  static vector<function<void()> > &ff(*(new vector<function<void()> >()));
  return ff;
}

void chdl::finally(function<void()> f) {
  final_funcs().push_back(f);
}

void chdl::call_final_funcs() {
  for (auto f : final_funcs()) f();
}
