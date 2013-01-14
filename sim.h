#ifndef __SIM_H
#define __SIM_H

#include <iostream>

namespace chdl {
  typedef unsigned long long cycle_t;

  void print_time(std::ostream&);
  cycle_t advance();

  void run(std::ostream &vcdout, cycle_t time);
};

#endif
