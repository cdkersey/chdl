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
  cycle_t advance(cdomain_handle_t cd, evaluator_t &e);

  evaluator_t &default_evaluator();

  void init_trans();
  void advance_trans();
  void recompute_logic_trans();
  void pre_tick_trans();
  void tick_trans();
  void tock_trans();
  void post_tock_trans();
  evaluator_t &trans_evaluator();

  void run_trans(std::ostream &vcdout, bool &stop, cycle_t max);
  void run_trans(std::ostream &vcdout, cycle_t max);

  void run(std::ostream &vcdout, bool &stop, unsigned threads = 1);
  void run(std::ostream &vcdout, bool &stop, cycle_t max, unsigned threads = 1);
  void run(std::ostream &vcdout, cycle_t time, unsigned threads = 1);
  void run(std::ostream &vcdout, std::function<bool()> end, unsigned threads=1);

  void gen_eval_all(evaluator_t &e, execbuf &b,
                    nodebuf_t &from, nodebuf_t &to);
  void gen_pre_tick_all(evaluator_t &e, execbuf &b,
                        nodebuf_t &from, nodebuf_t &to);
  void gen_tick_all(evaluator_t &e, execbuf &b,
                    nodebuf_t &from, nodebuf_t &to);
  void gen_tock_all(evaluator_t &e, execbuf &b,
                    nodebuf_t &from, nodebuf_t &to);
  void gen_post_tock_all(evaluator_t &e, execbuf &b,
                         nodebuf_t &from, nodebuf_t &to);

  void finally(std::function<void()> f);
  void call_final_funcs();
};
#endif
