#ifndef __SIM_H
#define __SIM_H

#include <iostream>

namespace chdl {
  typedef unsigned long long cycle_t;

  cycle_t sim_time();
  void print_time(std::ostream&);
  cycle_t advance(unsigned threads=1);

  void run(std::ostream &vcdout, cycle_t time, unsigned threads=1);
};

#endif
