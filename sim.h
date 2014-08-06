#ifndef __SIM_H
#define __SIM_H

#include <iostream>
#include <functional>
#include <vector>

#include "tickable.h"
#include "tap.h"
#include "cdomain.h"
#include "nodeimpl.h"

namespace chdl {
  typedef unsigned long long cycle_t;

  extern std::vector<cycle_t> now;

  cycle_t sim_time(cdomain_handle_t cd = 0);
  void print_time(std::ostream&);
  cycle_t advance(unsigned threads=1, cdomain_handle_t cd = 0);

  void run(std::ostream &vcdout, bool &stop, unsigned threads = 1);
  void run(std::ostream &vcdout, bool &stop, cycle_t max, unsigned threads = 1);
  void run(std::ostream &vcdout, cycle_t time, unsigned threads = 1);
  void run(std::ostream &vcdout, std::function<bool()> end, unsigned threads=1);

  void gen_eval_all(cdomain_handle_t h, execbuf &b,
                    nodebuf_t &from, nodebuf_t &to);

  void finally(std::function<void()> f);
  void call_final_funcs();
};
#endif
