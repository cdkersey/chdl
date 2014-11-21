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
#include "trisimpl.h"
#include "gatesimpl.h"
#include "memory.h"
#include "stopwatch.h"

using namespace chdl;
using namespace std;

#define DEBUG_TRANS

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

//  Register copy order
vector<nodeid_t> rcpy;

//  Short circuit sets and corresponding essential sets (candidates; not all)
map<nodeid_t, pair<set<nodeid_t>, set<nodeid_t> > > scs;

//  Clusters (output node, countained nodes, input nodes)
map<nodeid_t, pair<set<nodeid_t>, set<nodeid_t> > > clusters;
map<nodeid_t, nodeid_t> rclusters;
map<nodeid_t, vector<nodeid_t> > cluster_order;

//  Chunk of successor clusters for each cluster
map<nodeid_t, set<nodeid_t> > schunk;

//  Benefit counters for short-circuiting and successor-to-unchanged
vector<unsigned long> bc_sc, bc_suc;

// The default node buffer.
static nodebuf_t v;

// The default evaluator.
static evaluator_t e;

// The default exec buffer.
execbuf exb;

// The stopwatch collection
vector<pair<string, double> > times;
void push_time(const char *s) {
  times.push_back(make_pair<string, double>(s, 0));;
  stopwatch_start();
}

void pop_time() {
  times[times.size() - 1].second = stopwatch_stop();
}

// Find a valid register copy order.
void find_rcpy() {
  push_time("find_rcpy");

  // Get registers. Each register will have an entry in rg.
  set<nodeid_t> regs;
  get_reg_q_nodes(regs);

  // Register graph; for each q, a set of qs having it as d.
  map<nodeid_t, set<nodeid_t> > rg;

  // Generate the graph. Should be a DAG
  for (auto r : regs) {
    rg[r];
    regimpl *rp(static_cast<regimpl *>(nodes[r]));
    if (regs.count(rp->d)) rg[rp->d].insert(r);
  }

  #ifdef DEBUG_TRANS
  cout << "Register dependency graph (incoming edges):" << endl;
  for (auto &p : rg) {
    cout << "  " << p.first << ':';
    for (auto &r : p.second)
      cout << ' ' << r;
    cout << endl;
  }
  #endif

  // Build the register copy order.
  while (!rg.empty()) {
    for (auto p : rg) {
      if (p.second.empty()) {
        rcpy.push_back(p.first);
        rg.erase(p.first);
        for (auto q : rg)
          rg[q.first].erase(p.first);
        break;
      }
    }
  }

  #ifdef DEBUG_TRANS
  cout << "Register copy order:" << endl;
  for (auto r : rcpy)
    cout << "  " << r << endl;
  #endif

  pop_time();
}

// Recursively find all nodes that a node n depends on.
void dep_set(set<nodeid_t> &s, nodeid_t n, int max_depth = -1) {
  set<nodeid_t> frontier;
  frontier.insert(n);

  while (frontier.size() && max_depth--) {
    set<nodeid_t> next_frontier;
    for (auto n : frontier) {
      s.insert(n);
      for (auto src : nodes[n]->src)
        next_frontier.insert(src);
    }

    frontier = next_frontier;
  }
}

void find_scs() {
  push_time("find_scs");

  const unsigned DEPTH_LIMIT(10), MIN_SCS_SIZE(10);

  // First, build a successor table.
  map<nodeid_t, set<nodeid_t> > succ;
  for (nodeid_t n = 0; n < nodes.size(); ++n)
    for (auto s : nodes[n]->src)
      succ[s].insert(n);

  // Find all nand inputs and tristate enables and their associated nodes
  // Also find the gates that will be performing the short circuiting.
  map<nodeid_t, set<nodeid_t> > sc_nodes, sc_gates;
  for (nodeid_t n = 0; n < nodes.size(); ++n) {
    vector<node> &src(nodes[n]->src);                                        
    if (dynamic_cast<nandimpl *>(nodes[n])) {                                
      sc_nodes[src[0]].insert(src[1]);                                       
      sc_nodes[src[1]].insert(src[0]);                                       
      sc_gates[src[0]].insert(n);                                            
      sc_gates[src[1]].insert(n);                                            
    } else if (dynamic_cast<tristateimpl *>(nodes[n])) {                     
      // Odd nodes are enables                                               
      for (unsigned i = 0; i < src.size(); i += 2) {                         
        sc_nodes[src[i + 1]].insert(src[i]);                                 
        sc_gates[src[i + 1]].insert(n);                                      
      }                                                                      
    }                                                                        
  } 

  // For each nand input and tristate enable, find its short circuiting set.
  for (auto &p : sc_nodes) {
    // n is the enable/node for which we're finding the short circuiting set.
    nodeid_t n(p.first);
    set<nodeid_t> s(p.second);

    // Find the "essential set" of all nodes required to evaluate n.
    set<nodeid_t> e;
    dep_set(e, n, DEPTH_LIMIT);

    // Find all dependencies for the nodes in s and add them to s
    set<nodeid_t> s0(s);
    for (auto n : s0)
      dep_set(s, n, DEPTH_LIMIT);

    // Remove all of the nodes that are in the essential set.
    for (auto n : e) s.erase(n);

    // Remove all of the nodes with external successors until none remain.
    bool ext;
    do {
      ext = false;

      for (auto m : s) {
        for (auto ms : succ[m]) {
          // The gates responsible for the short-circuiting are allowed to be
          // successors, of course.
          if (sc_gates[n].count(ms)) continue;
          if (!s.count(ms)) {
            ext = true;
            break;
          }
        }
        if (ext) {
          s.erase(m);
          break;
        }
      }     
    } while(ext);

    if (!s.empty() && s.size() >= MIN_SCS_SIZE)
      scs[n] = make_pair(s, e);
  }

  #ifdef DEBUG_TRANS
  cout << "Short-circuit and essential set sizes for each node:" << endl;
  for (auto &p : scs)
    cout << "  " << p.first << ", " << p.second.first.size()
         << ", " << p.second.second.size() << endl;
  #endif

  pop_time();
}

void find_clusters() {
  push_time("find_clusters");

  const unsigned CLUSTER_INPUTS(3), MERGE_ITERATIONS(10);

  // Successor table
  map<nodeid_t, set<nodeid_t> > succ;
  for (nodeid_t n = 0; n < nodes.size(); ++n)
    for (auto s : nodes[n]->src)
      succ[s].insert(n);

  // This is a bottom-up algorithm. Start with single-node clusters.
  for (nodeid_t i = 0; i < nodes.size(); ++i) {
    set<nodeid_t> inputs;
    for (auto x : nodes[i]->src) inputs.insert(x);
    clusters[i] = make_pair(set<nodeid_t>{i}, inputs);
  }

  for (unsigned iter = 0; iter < MERGE_ITERATIONS; ++iter) {
    // For each cluster
    for (auto &c : clusters) {
      // For each input cluster
      for (auto d : c.second.second) {
        // All successors have to be in the cluster already.
        bool not_in_cluster(false);
        for (auto x : succ[d]) {
          if (!c.second.first.count(x)) {
            not_in_cluster = true;
            break;
          }
        }
        if (not_in_cluster) continue;
        
        // Try to merge.
        set<nodeid_t> inputs(c.second.second);
        inputs.erase(d);
        inputs.insert(clusters[d].second.begin(), clusters[d].second.end());
        if (inputs.size() > CLUSTER_INPUTS) continue;
      
        // Success. Perform the merge.
        c.second.second = inputs;
        c.second.first.insert(clusters[d].first.begin(),
                              clusters[d].first.end());
        clusters.erase(d);
        break;
      } 
    }
  }

  // Create reverse cluster lookup mapping node->cluster
  for (auto &c : clusters)
    for (auto n : c.second.first)
      rclusters[n] = c.first;

  #ifdef DEBUG_TRANS
  cout << "Clusters:" << endl;
  for (auto &c : clusters) {
    cout << "  " << c.first << ':';
    for (auto n : c.second.first) cout << ' ' << n;
    cout << '(';
    for (auto n : c.second.second) cout << ' ' << n;
    cout << ')' << endl;
  }
  #endif

  pop_time();
}

void find_cluster_orders() {
  push_time("find_cluster_orders");
  
  // Build yet another node successor table.
  map<nodeid_t, set<nodeid_t> > succ;
  for (nodeid_t n = 0; n < nodes.size(); ++n)
    for (auto s : nodes[n]->src)
      succ[s].insert(n);

  for (auto &c : clusters) {
    map<nodeid_t, int> depcount;
    for (auto n : c.second.first)
      for (auto s : succ[n])
        ++depcount[s];

    set<nodeid_t> can_push;
    for (auto n : c.second.first)
      if (depcount[n] == 0)
        can_push.insert(n);

    while (!can_push.empty()) {
      nodeid_t n(*can_push.begin());
      cluster_order[c.first].push_back(n);
      can_push.erase(n);
      for (auto s : succ[n])
        if (--depcount[s] == 0 && c.second.first.count(s))
          can_push.insert(s);
    }
  
    #ifdef DEBUG_TRANS
    if (cluster_order[c.first].size() != c.second.first.size()) {
      cout << "ERROR: Cluster size mismatch!" << endl;
      for (auto x : cluster_order[c.first]) cout << ' ' << x;
      cout << " vs ";
      for (auto x : c.second.first) cout << ' ' << x;
      cout << endl;
      abort();
    }
    #endif
  }

  pop_time();
}

void find_succ() {
  push_time("find_succ");

  // Find successor cluster sets for each cluster.
  for (auto &c : clusters)
    for (auto i : c.second.second)
      schunk[i].insert(c.first);

  #ifdef DEBUG_TRANS
  cout << "Successor chunks:" << endl;
  for (auto &p : schunk) {
    cout << "  " << p.first << ':';
    for (auto n : p.second)
      cout << ' ' << n;
    cout << endl;
  }
  #endif

  pop_time();
}

void clusterize(set<nodeid_t> &s) {
  // Prune all members of s that are not clusters.
  set<nodeid_t> non_cluster;
  for (auto x : s)
    if (!clusters.count(x))
      non_cluster.insert(x);

  // ... and remove them.
  for (auto x : non_cluster)
    s.erase(x);

  // Prune all members of s that are not fully contained in s
  set<nodeid_t> non_fully_contained;
  for (auto c : s) {
    for (auto x : clusters[c].first) {
      if (x == c) continue;
      if (!non_cluster.count(x))
        non_fully_contained.insert(x);
    }
  }

  for (auto x : non_fully_contained)
    s.erase(x);
}

void conservative_clusterize(set<nodeid_t> &s) {
  set<nodeid_t> new_s;

  for (auto n : s)
    new_s.insert(rclusters[n]);

  s = new_s;
}

void clusterize_scs() {
  push_time("clusterize_scs");

  #ifdef DEBUG_TRANS
    cout << "Short circuit set clusterization:" << endl;
  #endif

  for (auto &p : scs) {
    #ifdef DEBUG_TRANS
    cout << "scs for " << p.first << ": " << p.second.first.size()
         << '(' << p.second.second.size() << ')';
    #endif

    clusterize(p.second.first);
    conservative_clusterize(p.second.second);

    #ifdef DEBUG_TRANS
    cout << "->" << p.second.first.size()
         << '(' << p.second.second.size() << ')' << endl;
    #endif
  }

  pop_time();
}

void init_bcs() {
  push_time("init_bcs");

  for (nodeid_t i = 0; i < nodes.size(); ++i) {
    // Short circuit set.
    if (scs.count(i)) {
      if (scs[i].second.size() >= scs[i].first.size())
        bc_sc.push_back(0);
      else
        bc_sc.push_back(scs[i].first.size() - scs[i].second.size());
    } else {
      bc_sc.push_back(0);
    }

    // Successor
    if (schunk.count(i)) bc_suc.push_back(schunk[i].size());
    else bc_suc.push_back(0);
  }


  #ifdef DEBUG_TRANS
  cout << "Initial benefit counters:" << endl;
  for (nodeid_t i = 0; i < nodes.size(); ++i) {
    if (bc_sc[i]) cout << "  sc[" << i << "]: " << bc_sc[i] << endl;
    if (bc_suc[i]) cout << "  suc[" << i << "]: " << bc_suc[i] << endl;
  }
  #endif

  pop_time();
}

void chdl::init_trans() {
  // Resize our node buffer.
  v.resize(nodes.size());

  // e just returns a value from v:
  e = [](nodeid_t n) { return v[n]; };

  // Find valid register copy order.
  find_rcpy();

  // Find depth-limited short circuit sets and essential sets for each node
  // Select a set of candidate short circuit nodes
  find_scs();

  // Perform exclusive N-clustering, starting with SC nodes in s
  find_clusters();

  // Find valid eval orders for clusters.
  find_cluster_orders();

  // Compute successor chunks for each cluster.
  find_succ();

  // Convert short circuit node sets to clusters (wholly contained clusters)
  clusterize_scs();

  // Set initial benefit counter values for each node to short circuit set sizes
  // Set initial benefit counter values for each successor chunk to chunk sizes
  init_bcs();
}

evaluator_t &chdl::trans_evaluator() {
  return e;
}

void reg_trans() {
  push_time("reg_trans");

  for (auto r : rcpy)
    static_cast<regimpl*>(nodes[r])->gen_tick(e, exb, v, v);

  pop_time();
}

void gen_dep_table(map<nodeid_t, int> &t, set<nodeid_t> &evalable) {
  for (auto &p : clusters) {
    if (p.second.second.size() == 0) evalable.insert(p.first);
    t[p.first] = p.second.second.size();
  }

  #ifdef DEBUG_TRANS
  cout << "Initial evalable nodes: " << evalable.size() << endl;
  #endif
}

void update_evalable(nodeid_t n, map<nodeid_t, int> &t, set<nodeid_t> &evalable)
{
  evalable.erase(n);
  for (auto s : schunk[n])
    if (--t[s] == 0) evalable.insert(s);
}

void gen_cluster(nodeid_t c) {
  for (auto n : cluster_order[c]) {
    #ifdef DEBUG_TRANS
    cout << "Gen evaluate " << n << endl;
    #endif

    nodes[n]->gen_eval(e, exb, v);
    nodes[n]->gen_store_result(exb, v, v);
  }
}

void log_trans() {
  push_time("log_trans");

  map<nodeid_t, int> depcount;
  set<nodeid_t> evalable;
  gen_dep_table(depcount, evalable);

  // TODO: Include counter values when we do this.
  while (!evalable.empty()) {
    nodeid_t n = *(evalable.begin());

    gen_cluster(n);
    update_evalable(n, depcount, evalable);
  }

  pop_time();
}

void shift_bcs() {
  push_time("shift_bcs");
  for (auto &x : bc_sc) x >>= 1;
  for (auto &x : bc_suc) x >>= 1;
  pop_time();
}

void chdl::advance_trans(cdomain_handle_t cd) {
  const unsigned DYN_TRANS_INTERVAL(100000);

  if (now[0] % DYN_TRANS_INTERVAL == 0) {
    // Clear the execbuf.
    exb.clear();

    // Translate the registers.
    reg_trans();

    // Translate the logic.
    log_trans();

    // Finish off the translation with a ret instruction
    exb.push((char)0xc3);

    // Shift the counter values
    shift_bcs();
  }

  // Run the execbuf; one cycle of evaluation.
  exb();

  ++now[0];
}

void chdl::run_trans(std::ostream &vcdout, bool &stop, cycle_t max) {
  init_trans();

  print_vcd_header(vcdout);
  print_time(vcdout);
  for (unsigned i = 0; i < max; ++i) {
    advance_trans(0);
    print_taps(vcdout, e);
    print_time(vcdout);
  }
  print_time(vcdout);

  call_final_funcs();

  cout << "Stopwatch values:" << endl;
  for (auto &p : times)
    cout << "  " << p.first << ": " << p.second << "ms" << endl;    
}

void chdl::recompute_logic_trans(cdomain_handle_t cd) {
}

void chdl::pre_tick_trans(cdomain_handle_t cd) {
}

void chdl::tick_trans(cdomain_handle_t cd) {
}

void chdl::tock_trans(cdomain_handle_t cd) {
}

void chdl::post_tock_trans(cdomain_handle_t cd) {
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
