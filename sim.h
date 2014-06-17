#ifndef __SIM_H
#define __SIM_H

#include <iostream>

#include "tickable.h"
#include "tap.h"
#include "cdomain.h"

namespace chdl {
  typedef unsigned long long cycle_t;

  cycle_t sim_time();
  void print_time(std::ostream&);
  cycle_t advance(unsigned threads=1, cdomain_handle_t cd = 0);

  template <typename T>
    void run(std::ostream &vcdout, T end_conditon, unsigned threads = 1);

  void run(std::ostream &vcdout, bool &stop, unsigned threads = 1);
  void run(std::ostream &vcdout, bool &stop, cycle_t max, unsigned threads = 1);
  void run(std::ostream &vcdout, cycle_t time, unsigned threads = 1);
};

template <typename T>
  void chdl::run(std::ostream &vcdout, T end_condition, unsigned threads = 1)
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

#endif
