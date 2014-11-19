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

// Data for new algorithm:
//  Register copy order
//  Short circuit sets and corresponding essential sets (candidates; not all)
//  Clusters (output node, countained nodes, input nodes)
//  Successor chunks
//  Short-circuit sets and essential sets (as clusters)
//  Benefit counters for short-circuiting
//  Benefit counters for successor-to-unchanged

static nodebuf_t v;
static evaluator_t e;
execbuf exb;

void chdl::init_trans() {
  const unsigned CLUSTER_N(3);

  // Find valid register copy order.
  // Find depth-limited short circuit sets and essential sets for each node
  // Select a set of candidate short circuit nodes
  // Perform exclusive N-clustering, starting with SC nodes in s
  // Compute successor chunks for each cluster.
  // Convert short circuit node sets to clusters (wholly contained clusters)
  // Set initial benefit counter values for each node to short circuit set sizes
  // Set initial benefit counter values for each successor chunk to chunk sizes
}

evaluator_t &chdl::trans_evaluator() {
  if (now[0] % 2 == 0) return e0[0]; else return e1[0];
}

void chdl::advance_trans(cdomain_handle_t cd) {
  // TODO
  // Is it time to re-evaluate yet?
}

void chdl::run_trans(std::ostream &vcdout, bool &stop, cycle_t max) {
  init_trans();

  // TODO
  #if 0  

  print_vcd_header(vcdout);
  print_time(vcdout);
  for (unsigned i = 0; i < max && !stop; ++i) {
    for (unsigned cd = 0; cd < tickables().size(); ++cd) {
      if (i % tick_intervals()[cd] == tick_intervals()[cd] - 1) {
        if ((i & 1) == 0) {
          l0[cd]();
          if (cd == 0) print_taps(vcdout, e0[cd]);
          r0[cd]();
          ++now[cd];
        } else {
          l1[cd]();
          if (cd == 0) print_taps(vcdout, e1[cd]);
          r1[cd]();
          ++now[cd];
        }
        
      }
      print_time(vcdout);
    }
  }
  #endif

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
