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

static vector<cycle_t> memo_expires;
static vector<map<nodeid_t, bool> > memo;
static vector<evaluator_t*> default_evaluator_ptr;

evaluator_t &chdl::default_evaluator(cdomain_handle_t cd) {
  if (default_evaluator_ptr.size() <= cd) {
    default_evaluator_ptr.resize(cd + 1);
    memo.resize(cd + 1);
    memo_expires.resize(cd + 1);
  }

  if (default_evaluator_ptr[cd] == NULL) {
    default_evaluator_ptr[cd] = new evaluator_t(
      [cd](nodeid_t n){
        if (sim_time(cd) == memo_expires[cd]) {
          memo[cd].clear();
          ++memo_expires[cd];
        }
        if (!memo[cd].count(n))
          memo[cd][n] = nodes[n]->eval(*default_evaluator_ptr[cd]);
        return memo[cd][n];
      }
    );
  }

  return *default_evaluator_ptr[cd];
}

static void reset_default_evaluator() {
  for (auto p: default_evaluator_ptr) delete p;
  default_evaluator_ptr.clear();
  memo.clear();
  memo_expires.clear();
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

static vector<nodebuf_t> v0, v1;
static vector<evaluator_t> e0, e1;
static vector<execbuf> l0, r0, l1, r1, pre_tick_buf0, pre_tick_buf1, tick_buf0,
                       tick_buf1, tock_buf0, tock_buf1, post_tock_buf0,
                       post_tock_buf1;

void chdl::init_trans() {
  unsigned n_cdomains(tickables().size());
  l0.resize(n_cdomains);  r0.resize(n_cdomains);
  l1.resize(n_cdomains);  r1.resize(n_cdomains);
  v0.resize(n_cdomains);  v1.resize(n_cdomains);
  e0.resize(n_cdomains);  e1.resize(n_cdomains);
  pre_tick_buf0.resize(n_cdomains);
  pre_tick_buf1.resize(n_cdomains);
  tick_buf0.resize(n_cdomains);
  tick_buf1.resize(n_cdomains);
  tock_buf0.resize(n_cdomains);
  tock_buf1.resize(n_cdomains);
  post_tock_buf0.resize(n_cdomains);
  post_tock_buf1.resize(n_cdomains);

  for (cdomain_handle_t i = 0; i < n_cdomains; ++i) {
    l0[i].clear(); r0[i].clear();
    l1[i].clear(); r1[i].clear();

    pre_tick_buf0[i].clear();
    pre_tick_buf1[i].clear();
    tick_buf0[i].clear();
    tick_buf1[i].clear();
    tock_buf0[i].clear();
    tock_buf1[i].clear();
    post_tock_buf0[i].clear();
    post_tock_buf1[i].clear();

    v0[i].resize(nodes.size());
    v1[i].resize(nodes.size());

    e0[i] = [i](nodeid_t n){ return v0[i][n]; };
    e1[i] = [i](nodeid_t n){ return v1[i][n]; };

    gen_eval_all(e0[i], l0[i], v0[i], v1[i]);
    l0[i].push((char)0xc3);
    gen_pre_tick_all(i, e0[i], r0[i], v0[i], v1[i]);
    gen_tick_all(i, e0[i], r0[i], v0[i], v1[i]);
    gen_tock_all(i, e0[i], r0[i], v0[i], v1[i]);
    gen_post_tock_all(i, e0[i], r0[i], v0[i], v1[i]);
    r0[i].push((char)0xc3); // ret

    gen_pre_tick_all(i, e0[i], pre_tick_buf0[i], v0[i], v1[i]);
    pre_tick_buf0[i].push((char)0xc3); // ret
    gen_tick_all(i, e0[i], tick_buf0[i], v0[i], v1[i]);
    tick_buf0[i].push((char)0xc3); // ret
    gen_tock_all(i, e0[i], tock_buf0[i], v0[i], v1[i]);
    tock_buf0[i].push((char)0xc3); // ret
    gen_post_tock_all(i, e0[i], post_tock_buf0[i], v0[i], v1[i]);
    post_tock_buf0[i].push((char)0xc3); // ret

    gen_eval_all(e1[i], l1[i], v1[i], v0[i]);
    l1[i].push((char)0xc3);
    gen_pre_tick_all(i, e1[i], r1[i], v1[i], v0[i]);
    gen_tick_all(i, e1[i], r1[i], v1[i], v0[i]);
    gen_tock_all(i, e1[i], r1[i], v1[i], v0[i]);
    gen_post_tock_all(i, e1[i], r1[i], v1[i], v0[i]);
    r1[i].push((char)0xc3); // ret

    gen_pre_tick_all(i, e1[i], pre_tick_buf1[i], v1[i], v0[i]);
    pre_tick_buf1[i].push((char)0xc3); // ret
    gen_tick_all(i, e1[i], tick_buf1[i], v1[i], v0[i]);
    tick_buf1[i].push((char)0xc3); // ret
    gen_tock_all(i, e1[i], tock_buf1[i], v1[i], v0[i]);
    tock_buf1[i].push((char)0xc3); // ret
    gen_post_tock_all(i, e1[i], post_tock_buf1[i], v1[i], v0[i]);
    post_tock_buf1[i].push((char)0xc3); // ret
  }
}

evaluator_t &chdl::trans_evaluator() {
  if (now[0] % 2 == 0) return e0[0]; else return e1[0];
}

void chdl::advance_trans(cdomain_handle_t cd) {
  if (!(now[cd] & 1)) {
    l0[cd]();
    r0[cd]();
    ++now[0];
  } else {
    l1[cd]();
    r1[cd]();
    ++now[cd];
  }
}

void chdl::run_trans(std::ostream &vcdout, bool &stop, cycle_t max) {
  init_trans();

  print_vcd_header(vcdout);
  print_time(vcdout);
  for (unsigned i = 0; i < max && !stop; ++i) {
    for (unsigned cd = 0; cd < tickables().size(); ++cd) {
      if (i % tick_intervals()[cd] == 0) {
        if ((i & 1) == 0) {
          l0[cd]();
          print_taps(vcdout, e0[cd]);
          r0[cd]();
          ++now[cd];
        print_time(vcdout);
        } else {
          l1[cd]();
          print_taps(vcdout, e1[cd]);
          r1[cd]();
          ++now[cd];
          print_time(vcdout);
        }
      }
    }
  }

  call_final_funcs();
}

void chdl::recompute_logic_trans(cdomain_handle_t cd) {
  if ((now[0] % 2) == 0)
    l0[cd]();
  else
    l1[cd]();
}

void chdl::pre_tick_trans(cdomain_handle_t cd) {
  if ((now[0] % 2) == 0) {
    l0[cd]();
    pre_tick_buf1[0]();
  } else {
    l1[cd]();
    pre_tick_buf0[0]();
  }
}

void chdl::tick_trans(cdomain_handle_t cd) {
  if ((now[0] % 2) == 0) {
    tick_buf0[cd]();
  } else {
    tick_buf1[cd]();
  }
}

void chdl::tock_trans(cdomain_handle_t cd) {
  if ((now[cd] % 2) == 0)
    tock_buf0[cd]();
  else
    tock_buf1[cd]();
}

void chdl::post_tock_trans(cdomain_handle_t cd) {
  if ((now[cd] % 2) == 0)
    post_tock_buf0[cd]();
  else
    post_tock_buf1[cd]();
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

void chdl::gen_pre_tick_all(cdomain_handle_t cd, evaluator_t &e, execbuf &b,
                            nodebuf_t &from, nodebuf_t &to)
{
  for (auto t : tickables()[cd]) t->gen_pre_tick(e, b, from, to);
}

void chdl::gen_tick_all(cdomain_handle_t cd, evaluator_t &e, execbuf &b,
                        nodebuf_t &from, nodebuf_t &to)
{
  for (auto t : tickables()[cd]) t->gen_tick(e, b, from, to);
}

void chdl::gen_tock_all(cdomain_handle_t cd, evaluator_t &e, execbuf &b,
                        nodebuf_t &from, nodebuf_t &to)
{
  for (auto t : tickables()[cd]) t->gen_tock(e, b, from, to);
}

void chdl::gen_post_tock_all(cdomain_handle_t cd, evaluator_t &e, execbuf &b,
                             nodebuf_t &from, nodebuf_t &to)
{
  for (auto t : tickables()[cd]) t->gen_post_tock(e, b, from, to);
}
