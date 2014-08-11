#include <iostream>
#include <functional>
#include <vector>
#include <map>

#include "sim.h"
#include "tickable.h"
#include "tap.h"
#include "reset.h"
#include "cdomain.h"
#include "regimpl.h"
#include "memory.h"

using namespace chdl;
using namespace std;

vector<cycle_t> chdl::now{0};
static void reset_now() { now = vector<cycle_t>(1); }
CHDL_REGISTER_RESET(reset_now);

cycle_t chdl::sim_time(cdomain_handle_t cd) { return now[cd]; }

static cycle_t memo_expires = 0;
static map<nodeid_t, bool> memo;
static evaluator_t *default_evaluator_ptr = NULL;

evaluator_t &chdl::default_evaluator() {
  if (!default_evaluator_ptr) {
    default_evaluator_ptr = new evaluator_t(
      [](nodeid_t n){
        if (sim_time(0) == memo_expires) { memo.clear(); ++memo_expires; }
        if (!memo.count(n))
          memo[n] = nodes[n]->eval(*default_evaluator_ptr);
        return memo[n];
      }
    );
  }

  return *default_evaluator_ptr;
}

static void reset_default_evaluator() {
  delete(default_evaluator_ptr);
  default_evaluator_ptr = NULL;
  memo.clear();
  memo_expires = 0;
}
CHDL_REGISTER_RESET(reset_default_evaluator);

cycle_t chdl::advance(cdomain_handle_t cd, evaluator_t &e) {
  for (auto &t : tickables()[cd]) t->pre_tick(e);
  for (auto &t : tickables()[cd]) t->tick(e);
  for (auto &t : tickables()[cd]) t->tock(e);
  for (auto &t : tickables()[cd]) t->post_tock(e);

  ++now[cd];

  return now[cd];
}

void chdl::print_time(ostream &out) {
  out << '#' << now[0] << endl;  
}

void chdl::run(ostream &vcdout, function<bool()> end_condition,
               unsigned threads)
{
  // Memoizing evaluator
  evaluator_t e;
  map<nodeid_t, bool> memo;
  e = [&e, &memo](nodeid_t n) {
    if (!memo.count(n)) {
      memo[n] = nodes[n]->eval(e);
    }
    return memo[n];
  };

  vector<unsigned> &ti(tick_intervals());

  print_vcd_header(vcdout);
  print_time(vcdout);
  do {
    print_taps(vcdout, e);
    for (unsigned j = 0; j < ti.size(); ++j)
      if (sim_time()%ti[j] == 0) advance(j, e);
    memo.clear();
    print_time(vcdout);
  } while (!end_condition());

  call_final_funcs();
}

void dump(nodebuf_t x) {
  unsigned col = 0;
  for (auto v : x) {
    if (col && (col % 32 == 0)) cout << endl;
    col++;
    cout << ' ' << v;
  }
  cout << endl;
}

static nodebuf_t v0, v1;
static evaluator_t e0, e1;
static execbuf l0, r0, l1, r1, pre_tick_buf0, pre_tick_buf1, tick_buf0,
               tick_buf1, tock_buf0, tock_buf1, post_tock_buf0,
               post_tock_buf1;

void chdl::init_trans() {
  l0.clear(); r0.clear();
  l1.clear(); r1.clear();

  pre_tick_buf0.clear();
  pre_tick_buf1.clear();
  tick_buf0.clear();
  tick_buf1.clear();
  tock_buf0.clear();
  tock_buf1.clear();
  post_tock_buf0.clear();
  post_tock_buf1.clear();

  v0.resize(nodes.size());
  v1.resize(nodes.size());

  e0 = [](nodeid_t n){ return v0[n]; };
  e1 = [](nodeid_t n){ return v1[n]; };

  gen_eval_all(e0, l0, v0, v1);
  l0.push((char)0xc3);
  gen_pre_tick_all(e0, r0, v0, v1);
  gen_tick_all(e0, r0, v0, v1);
  gen_tock_all(e0, r0, v0, v1);
  gen_post_tock_all(e0, r0, v0, v1);
  r0.push((char)0xc3); // ret

  gen_pre_tick_all(e0, pre_tick_buf0, v0, v1);
  pre_tick_buf0.push((char)0xc3); // ret
  gen_tick_all(e0, tick_buf0, v0, v1);
  tick_buf0.push((char)0xc3); // ret
  gen_tock_all(e0, tock_buf0, v0, v1);
  tock_buf0.push((char)0xc3); // ret
  gen_post_tock_all(e0, post_tock_buf0, v0, v1);
  post_tock_buf0.push((char)0xc3); // ret

  gen_eval_all(e1, l1, v1, v0);
  l1.push((char)0xc3);
  gen_pre_tick_all(e1, r1, v1, v0);
  gen_tick_all(e1, r1, v1, v0);
  gen_tock_all(e1, r1, v1, v0);
  gen_post_tock_all(e1, r1, v1, v0);
  r1.push((char)0xc3); // ret

  gen_pre_tick_all(e1, pre_tick_buf1, v1, v0);
  pre_tick_buf1.push((char)0xc3); // ret
  gen_tick_all(e1, tick_buf1, v1, v0);
  tick_buf1.push((char)0xc3); // ret
  gen_tock_all(e1, tock_buf1, v1, v0);
  tock_buf1.push((char)0xc3); // ret
  gen_post_tock_all(e1, post_tock_buf1, v1, v0);
  post_tock_buf1.push((char)0xc3); // ret
}

evaluator_t &chdl::trans_evaluator() {
  if (now[0] % 2 == 0) return e0; else return e1;
}

void chdl::advance_trans() {
  if (!(now[0] & 1)) {
    l0();
    r0();
    ++now[0];
  } else {
    l1();
    r1();
    ++now[0];
  }
}

void chdl::run_trans(std::ostream &vcdout, bool &stop, cycle_t max) {
  init_trans();

  print_vcd_header(vcdout);
  print_time(vcdout);
  for (unsigned i = 0; i < max && !stop; ++i) {
    l0();
    print_taps(vcdout, e0);
    r0();
    ++now[0];
    print_time(vcdout);

    if (stop || ++i == max) break;

    l1();
    print_taps(vcdout, e1);
    r1();
    ++now[0];
    print_time(vcdout);
  }

  call_final_funcs();
}

void chdl::recompute_logic_trans() {
  if ((now[0] % 2) == 0)
    l0();
  else
    l1();
}

void chdl::pre_tick_trans() {
  if ((now[0] % 2) == 0) {
    l0();
    pre_tick_buf1();
  } else {
    l1();
    pre_tick_buf0();
  }
}

void chdl::tick_trans() {
  if ((now[0] % 2) == 0) {
    tick_buf0();
  } else {
    tick_buf1();
  }
}

void chdl::tock_trans() {
  if ((now[0] % 2) == 0)
    tock_buf0();
  else
    tock_buf1();
}

void chdl::post_tock_trans() {
  if ((now[0] % 2) == 0)
    post_tock_buf0();
  else
    post_tock_buf1();
}

void chdl::run_trans(std::ostream &vcdout, cycle_t max) {
  bool stop(false);
  run_trans(vcdout, stop, max);
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

// Get all of the destination nodes; the nodes which may not have successors.
void get_dest_nodes(set<nodeid_t> &s) {
  get_tap_nodes(s);
  get_reg_nodes(s);
  get_mem_nodes(s);
}

// Get all of the origin nodes; the nodes without sources.
void get_src_nodes(set<nodeid_t> &s) {
  for (nodeid_t n = 0; n < nodes.size(); ++n)
    if (nodes[n]->src.size() == 0) s.insert(n);
}

void get_logic_layers(map<unsigned, set<nodeid_t> > &ll) {
  map<nodeid_t, unsigned> ll_r;
  set<nodeid_t> f;
  get_dest_nodes(f);
  unsigned l = 0;
  while (!f.empty()) {
    set<nodeid_t> next_f;
    for (auto n : f) {
      ll_r[n] = l;
      for (auto s : nodes[n]->src)
        next_f.insert(s);
    }

    f = next_f;
    ++l;
  }

  // We compute the layes in reverse, starting with the destination, but we want
  // the output to be a number corresponding to the order in which the node
  // values should be computed.
  unsigned max_l = l-1;
  for (auto p : ll_r)
    ll[max_l - p.second].insert(p.first);
}

void chdl::gen_eval_all(evaluator_t &e, execbuf &b,
                        nodebuf_t &from, nodebuf_t &to)
{
  map<unsigned, set<nodeid_t> > ll;
  get_logic_layers(ll);  

  for (auto p : ll) {
    for (auto n : p.second) {
      nodes[n]->gen_eval(e, b, from);
      nodes[n]->gen_store_result(b, from, to);
    }
  }
}

void chdl::gen_pre_tick_all(evaluator_t &e, execbuf &b,
                            nodebuf_t &from, nodebuf_t &to)
{
  for (auto t : tickables()[0]) t->gen_pre_tick(e, b, from, to);
}

void chdl::gen_tick_all(evaluator_t &e, execbuf &b,
                        nodebuf_t &from, nodebuf_t &to)
{
  for (auto t : tickables()[0]) t->gen_tick(e, b, from, to);
}

void chdl::gen_tock_all(evaluator_t &e, execbuf &b,
                        nodebuf_t &from, nodebuf_t &to)
{
  for (auto t : tickables()[0]) t->gen_tock(e, b, from, to);
}

void chdl::gen_post_tock_all(evaluator_t &e, execbuf &b,
                             nodebuf_t &from, nodebuf_t &to)
{
  for (auto t : tickables()[0]) t->gen_post_tock(e, b, from, to);
}
