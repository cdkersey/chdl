#include <iostream>
#include <thread>

#include "sim.h"
#include "tickable.h"
#include "tap.h"

using namespace chdl;
using namespace std;

static cycle_t now = 0;

cycle_t chdl::sim_time() { return now; }

void tick_thread(unsigned tid, unsigned threads) {
  size_t n(tickables.size()), step((n + threads - 1)/threads),
         a(tid * step), b((tid+1)*step > n ? n : (tid+1)*step);
  
  for (size_t i = a; i < b; ++i) tickables[i]->tick();
}

void tock_thread(unsigned tid, unsigned threads) {
  size_t n(tickables.size()), step((n + threads - 1)/threads),
         a(tid * step), b((tid+1)*step > n ? n : (tid+1)*step);
  
  for (size_t i = a; i < b; ++i) tickables[i]->tock();
}

cycle_t chdl::advance(unsigned threads) {
  if (threads == 1) {
    for (size_t i = 0; i < tickables.size(); ++i) tickables[i]->tick();
    for (size_t i = 0; i < tickables.size(); ++i) tickables[i]->tock();
  } else {
    vector<thread> t(threads);

    for (size_t i = 0; i < threads; ++i)
      t[i] = thread(tick_thread, i, threads);
    for (size_t i = 0; i < threads; ++i)
      t[i].join();

    for (size_t i = 0; i < threads; ++i)
      t[i] = thread(tock_thread, i, threads);
    for (size_t i = 0; i < threads; ++i)
      t[i].join();
  }

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
