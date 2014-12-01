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
#include "nodeimpl.h"
#include "litimpl.h"
#include "memory.h"
#include "stopwatch.h"

using namespace chdl;
using namespace std;

#define DEBUG_TRANS
#define INC_VISITED
#define NO_VCD
// #define FINAL_REPORT

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
    if (col && (col % 32) == 0) cout << endl;
    col++;
    cout << ' ' << v;
  }
  cout << endl;
}

// The non-register tickables that must be called by the translator.
vector<tickable*> trans_tickables;

// Logic-layer of each node, size of each logic layer
map<nodeid_t, int> ll;
vector<nodeid_t> ll_size;

// Static clusters for each logic layer
map<int, set<nodeid_t> > static_clusters;

 // location in TC of beginning and current pos in logic layer call bufs
vector<int> ll_offset, ll_base_offset;
vector<vector<char*> > ll_bufs;
vector<char*> ll_buf, ll_pos;

// ...
vector<unsigned> visited, eval_cyc;

//  Register copy order
vector<nodeid_t> rcpy;

// Successor table
map<nodeid_t, set<nodeid_t> > succ;

//  Short circuit sets and corresponding essential sets (candidates; not all)
map<nodeid_t, pair<set<nodeid_t>, set<nodeid_t> > > scs;

//  Clusters (output node, countained nodes, input nodes)
map<nodeid_t, pair<set<nodeid_t>, set<nodeid_t> > > clusters;
map<nodeid_t, nodeid_t> rclusters;
map<nodeid_t, vector<nodeid_t> > cluster_order;

//  Chunk of successor clusters for each cluster
map<nodeid_t, set<nodeid_t> > schunk;

// The default node buffer.
static nodebuf_t v;

// The default evaluator.
static evaluator_t e;

// The default exec buffer.
execbuf exb;

// Cluster address tables
map<nodeid_t, int> implptr;
map<nodeid_t, set<int> > fixup;
map<int, set<int> > ll_fixup;

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
  //static map<nodeid_t, set<nodeid_t> > ds;
  //static map<nodeid_t, int> depth;

  s.insert(n);

  //if (max_depth == 0) return;

  //if (ds.count(n) && depth[n] >= max_depth) {
  //  for (auto &x : ds[n]) s.insert(x);
  //} else {
  //  depth[n] = max_depth;
  //  ds[n].insert(n);

    for (auto src : nodes[n]->src) {
      set<nodeid_t> new_el;
      dep_set(new_el, src, max_depth - 1);
      
      for (auto x : new_el) {
        s.insert(x);
  //      ds[n].insert(x);
      }
    }
  //}

  #ifdef DEBUG_TRANS
  // cout << "Dep set for " << n << ": " << ds[n].size() << " nodes." << endl;
  #endif
}

void find_succ_nodes() {
  push_time("find_succ_nodes");

  // First, build a successor table.
  for (nodeid_t n = 0; n < nodes.size(); ++n)
    for (auto s : nodes[n]->src)
      succ[s].insert(n);

  pop_time();
}

void find_scs() {
  push_time("find_scs");

  const unsigned DEPTH_LIMIT(5), MIN_SCS_SIZE(10);

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
    cout << "Finding scs for " << p.first << endl;
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

  const unsigned CLUSTER_INPUTS(5), MERGE_ITERATIONS(25);

  // Outputs and d nodes break clusters too.
  set<nodeid_t> out_and_d;
  get_reg_d_nodes(out_and_d);
  get_tap_nodes(out_and_d);
  
  // This is a bottom-up algorithm. Start with single-node clusters.
  for (nodeid_t i = 0; i < nodes.size(); ++i) {
    if (!dynamic_cast<nandimpl*>(nodes[i]) &&
        !dynamic_cast<invimpl*>(nodes[i]) &&
        !dynamic_cast<tristateimpl*>(nodes[i])) continue; // No non-logic

    set<nodeid_t> inputs;
    for (auto x : nodes[i]->src) inputs.insert(x);
    clusters[i] = make_pair(set<nodeid_t>{i}, inputs);
  }

  for (unsigned iter = 0; iter < MERGE_ITERATIONS; ++iter) {
    // For each cluster
    for (auto &c : clusters) {
      // For each input cluster
      for (auto d : c.second.second) {
        if (!dynamic_cast<nandimpl*>(nodes[d]) &&
            !dynamic_cast<invimpl*>(nodes[d])) continue; // No non-logic or tris

        // Can't merge in something that is an output or register input
	if (out_and_d.count(d)) continue;
	
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

void find_logic_layers() {
  push_time("find_logic_layers");

  int cur_ll(0);
  set<nodeid_t> frontier;
  get_reg_q_nodes(frontier);
  get_mem_q_nodes(frontier);

  // Simple dataflow algorithm. Automatically assigns max possible layer to each
  // node.
  while (!frontier.empty()) {
    set<nodeid_t> next_frontier;
    for (auto x : frontier) {
      ll[x] = cur_ll;
      for (auto c : schunk[x])
        next_frontier.insert(c);
    }
    frontier = next_frontier;
    cur_ll++;
  }

  // Determine the logic layer sizes by counting.
  ll_size.resize(cur_ll);
  for (auto &p : ll) ++ll_size[p.second];

  #ifdef DEBUG_TRANS
  cout << "Logic layers:" << endl;
  for (unsigned l = 0; l < ll_size.size(); ++l)
    cout << l << ": " << ll_size[l] << endl;
  #endif

  pop_time();
}

void find_cluster_orders() {
  push_time("find_cluster_orders");
  
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

void copy_tickables() {
  push_time("copy_tickables");

  for (auto t : tickables()[0]) {
    if (dynamic_cast<regimpl*>(t)) continue; // Regs need not apply
    trans_tickables.push_back(t);
  }

  pop_time();
}

void chdl::init_trans() {
  // Resize our node buffer.
  v.resize(nodes.size());
  visited.resize(nodes.size());
  eval_cyc.resize(nodes.size(), -1);

  // e just returns a value from v:
  e = [](nodeid_t n) { return v[n]; };

  // We have an array of non-register tickables. Fill it.
  copy_tickables();

  // A lot of our analyses depend on this.
  find_succ_nodes();

  // Find valid register copy order.
  find_rcpy();

  // Perform exclusive N-clustering, starting with SC nodes
  find_clusters();

  #if 0
  // Find valid eval orders for clusters.
  find_cluster_orders();
  #endif
  
  // Compute successor chunks for each cluster.
  find_succ();

  // Find logic layer for each cluster
  find_logic_layers();

  // Size all of the logic layer centric things
  ll_buf.resize(ll_size.size());
  ll_pos.resize(ll_size.size());
  ll_offset.resize(ll_size.size());
  ll_base_offset.resize(ll_size.size());

  // Evaluate initial values for v.
  for (unsigned i = 0; i < nodes.size(); ++i)
    v[i] = nodes[i]->eval(default_evaluator(0));

  // An initial tick for tickables.
  for (auto t : trans_tickables) t->tick(e);
}

evaluator_t &chdl::trans_evaluator() {
  return e;
}

void gen_dep_table(map<nodeid_t, int> &t, set<nodeid_t> &evalable)
{
  for (auto &p : clusters) {
    if (p.second.second.size() == 0) {
      evalable.insert(p.first);
    }
    t[p.first] = p.second.second.size();
  }

  #ifdef DEBUG_TRANS
  cout << "Initial evalable nodes: " << evalable.size() << endl;
  #endif
}

void update_evalable(nodeid_t n, map<nodeid_t, int> &t,
                     set<nodeid_t> &evalable)
{
  evalable.erase(n);
  for (auto s : schunk[n]) {
    if (--t[s] == 0)
      evalable.insert(s);
  }
}

size_t trans_gencall(nodeid_t n) {
    // The following code generates code which generates (that is not a typo)
    // a call to a cluster evaluation. The generated machine code is:
    // 0x48 0xb8 [8B call dest] ff d0. This code is placed at ll_pos[i], after
    // which ll_pos[i] is incremented. This code is skipped if the value at
    // eval_cyc[n] is equal to the value in %edi.

    // if (%rdi == eval_cyc[n]) skip
    exb.push(char(0x48)); // mov &eval_cyc[n], %rax
    exb.push(char(0xb8));
    exb.push((void*)&eval_cyc[n]);

    exb.push(char(0x3b)); // cmp (%rax),%edi
    exb.push(char(0x38));

    // exb.push(char(0x8b)); // mov (%rax),%ebx
    // exb.push(char(0x18));

    // exb.push(char(0x39)); // cmp %ebx,%edi
    // exb.push(char(0xdf));

    exb.push(char(0x74)); // je 35 (skip rest of impl)
    exb.push(char(35));

    // eval_cyc[n] = %edi
    exb.push(char(0x89)); // mov %edi,(%rax)
    exb.push(char(0x38));

    // pos = ll_pos[ll[n]];
    exb.push(char(0x48)); // mov &ll_pos[ll[n]], %rbx
    exb.push(char(0xbb));
    exb.push((void*)&ll_pos[ll[n]]);

    exb.push(char(0x48)); // mov (%rbx),%rax
    exb.push(char(0x8b));
    exb.push(char(0x03));

    // *(short*)pos = 0xb848
    //exb.push(char(0x66)); // movw 0xb848,(%rax)
    //exb.push(char(0xc7)); 
    //exb.push(char(0x00));
    //exb.push(short(0xb848));

    // *(void*)pos = CLUSTERIMPLFUNC
    exb.push(char(0x48)); // mov CLUSTERIMPLFUNC, %rcx
    exb.push(char(0xb9));
    fixup[n].insert(exb.push_future<void*>());

    exb.push(char(0x48)); // mov %rcx,(%rax)
    exb.push(char(0x89));
    exb.push(char(0x08));

    // *(short*)pos = 0xd0ff
    //exb.push(char(0x66)); // movw 0xd0ff,10(%rax)
    //exb.push(char(0xc7));
    //exb.push(char(0x40));
    //exb.push(char(0x0a));
    //exb.push(short(0xd0ff));

    // pos += 8
    exb.push(char(0x48)); // add 8,%rax
    exb.push(char(0x83));
    exb.push(char(0xc0));
    exb.push(char(8));

    // ll_pos[ll[n]] = pos
    exb.push(char(0x48)); // mov %rax,(%rbx)
    exb.push(char(0x89));
    exb.push(char(0x03));

    return /*51*/49; // Return size of call in bytes
}


// Translate a specific register
void trans_reg(nodeid_t r) {
  implptr[r] = exb.get_pos();

  regimpl* rp(static_cast<regimpl*>(nodes[r]));

  #ifdef INC_VISITED
  exb.push(char(0x48)); // mov &visited[r], %rbx
  exb.push(char(0xbb));
  exb.push((void*)&visited[r]);

  exb.push(char(0xff)); // incl (%rbx)
  exb.push(char(0x03));
  #endif

  exb.push(char(0x48)); // mov &q, %rdx
  exb.push(char(0xba));
  exb.push((void*)&v[r]);

  exb.push(char(0x8b)); // mov (%rdx), %eax
  exb.push(char(0x02));

  exb.push(char(0x48)); // mov &d, %rbx
  exb.push(char(0xbb));
  exb.push((void*)&v[rp->d]);

  exb.push(char(0x8b)); // mov (%rbx), %ecx
  exb.push(char(0x0b));

  exb.push(char(0x39)); // cmp %eax, %ecx
  exb.push(char(0xc1));

  int skip_offset;
  if (!schunk[r].empty()) {
    exb.push(char(0x0f)); // je finish
    exb.push(char(0x84));
    skip_offset = (exb.push_future<unsigned>());
  }

  exb.push(char(0x89)); // mov %ecx, (%rdx)
  exb.push(char(0x0a));

  if (!schunk[r].empty()) {
    unsigned offset_count = 2;
     for (auto c : schunk[r])
      offset_count += trans_gencall(c);

    exb.push(skip_offset, offset_count);
  }
}

unsigned gen_tt_internal(nodeid_t out, map<nodeid_t, unsigned> &inputs) {
  if (inputs.count(out)) return inputs[out];

  auto &s(nodes[out]->src);
  
  if (dynamic_cast<nandimpl*>(nodes[out])) {
    return ~(gen_tt_internal(s[0], inputs) & gen_tt_internal(s[1], inputs));
  } else if (dynamic_cast<invimpl*>(nodes[out])) {
    return ~(gen_tt_internal(s[0], inputs));
  } else if (dynamic_cast<litimpl*>(nodes[out])) {
    #ifdef DEBUG_TRANS
    cout << "Lit in cluster! Strange." << endl;
    #endif
    return nodes[out]->eval(default_evaluator(0)) ? 0xffffffff : 0x00000000;
  } else {
    cout << "Error: node " << out << " neither inverter nor nand." << endl;
    abort();
  }
}

unsigned gen_tt(nodeid_t out, vector<nodeid_t> &inputs) {
  // Create the basic truth tables.
  vector<unsigned> pattern = {0xaaaaaaaa,
			      0xcccccccc,
			      0xf0f0f0f0,
			      0xff00ff00,
			      0xffff0000 };

  map<nodeid_t, unsigned> inputmap;
  
  for (unsigned i = 0; i < inputs.size(); ++i)
    inputmap[inputs[i]] = pattern[i];

  unsigned tt = gen_tt_internal(out, inputmap);

  #if 0
  cout << "Truth table for cluster " << out << ": " << hex << tt << dec << endl;
  #endif
  
  return tt;
}

void trans_tt(vector<nodeid_t> &inputs,  unsigned tt) {
  // Clear %ecx
  exb.push(char(0x31)); // xor %ecx,%ecx
  exb.push(char(0xc9));
  
  // Load inputs into bits of %ecx
  for (int i = 0; i < inputs.size(); ++i) {
    if (i != 0) {
      exb.push(char(0xd1)); // shl %ecx
      exb.push(char(0xe1));
    }
    
    exb.push(char(0x48)); // mov &v[inputs[i]], %rax
    exb.push(char(0xb8));
    exb.push((void*)&v[inputs[inputs.size() - i - 1]]);

    if (i == 0) {
      exb.push(char(0x8b)); // mov (%rax), %ecx
      exb.push(char(0x08));
    } else {
      exb.push(char(0x0b)); // or (%rax),%ecx
      exb.push(char(0x08));
    }
  }

  exb.push(char(0xb8)); // mov TRUTHTABLE_BITS, %eax
  exb.push(tt);

  exb.push(char(0xd3)); // shr %cl,%eax
  exb.push(char(0xe8));
  
  exb.push(char(0x83)); // and $1,%eax
  exb.push(char(0xe0));
  exb.push(char(0x01));
}

// Translate a specific cluster
void trans_cluster(nodeid_t c, bool stat = false) {
  if (!stat) implptr[c] = exb.get_pos();

  set<nodeid_t> non_stat_succ;
  for (auto s : schunk[c])
    if (!static_clusters[ll[c]+1].count(s))
      non_stat_succ.insert(s);
  
  #ifdef INC_VISITED
  exb.push(char(0x48)); // mov &visited[r], %rbx
  exb.push(char(0xbb));
  exb.push((void*)&visited[c]);

  exb.push(char(0xff)); // incl (%rbx)
  exb.push(char(0x03));
  #endif

  exb.push(char(0x48)); // mov &out, %rbx
  exb.push(char(0xbb));
  exb.push((void*)&v[c]);

  exb.push(char(0x8b)); // mov (%rbx),%edx
  exb.push(char(0x13));

  // Translate the gates, leaving the output in %eax
  if (clusters[c].first.size() == 1) {
    // Might be slightly faster not to use truth tables for single-gate chunks.
    auto &s(nodes[c]->src);
    if (dynamic_cast<nandimpl*>(nodes[c])) {
      exb.push(char(0x48)); // mov &v[src[0]],%rcx
      exb.push(char(0xb9));
      exb.push((void*)&v[s[0]]);

      exb.push(char(0x8b)); // mov (%rcx),%ecx
      exb.push(char(0x09));
      
      exb.push(char(0x48)); // mov &v[src[1]],%rax
      exb.push(char(0xb8));
      exb.push((void*)&v[s[1]]);
      
      exb.push(char(0x8b)); // mov (%rax),%eax
      exb.push(char(0x00));
      
      exb.push(char(0x21)); // and %ecx,%eax
      exb.push(char(0xc8));

      exb.push(char(0x83)); // xor $1,%eax
      exb.push(char(0xf0));
      exb.push(char(0x01));
    } else if (dynamic_cast<invimpl*>(nodes[c])) {
      exb.push(char(0x48)); // mov %v[src[0]],%rax
      exb.push(char(0xb8));
      exb.push((void*)&v[s[0]]);
      
      exb.push(char(0x8b)); // mov (%rax),%eax
      exb.push(char(0x00));
      
      exb.push(char(0x83)); // xor $1,%eax
      exb.push(char(0xf0));
      exb.push(char(0x01));
    } else {
      cout << "SINGLE GATE NEITHER NAND NOR INV!\n";
      nodes[c]->gen_eval(e, exb, v);
    }
  } else {
    vector<nodeid_t> in_v;
    for (auto &i : clusters[c].second) in_v.push_back(i);
    trans_tt(in_v, gen_tt(c, in_v));
  }

  if (!non_stat_succ.empty()) {
    exb.push(char(0x39)); // cmp %eax, %edx
    exb.push(char(0xc2));

    exb.push(char(0x0f)); // je finish
    exb.push(char(0x84));
    int skip_offset(exb.push_future<unsigned>());

    exb.push(char(0x89)); // mov %eax,(%rbx) // Store new result if changed.
    exb.push(char(0x03));

    unsigned offset_count(0);
    for (auto s : non_stat_succ) {
      offset_count += trans_gencall(s);
    }

    exb.push(skip_offset, offset_count);
  } else {
    exb.push(char(0x89)); // mov %eax,(%rbx) // Store new result if changed.
    exb.push(char(0x03));
  }

  if (!stat) exb.push(char(0xc3));
}

void trans_eval(nodeid_t n) {
  cout << "trans eval " << n << endl;

  exb.push(char(0x48)); // mov &out, %rbx
  exb.push(char(0xbb));
  exb.push((void*)&v[n]);

  exb.push(char(0x8b)); // mov (%rbx),%ecx
  exb.push(char(0x0b));

  nodes[n]->gen_eval(e, exb, v);
  nodes[n]->gen_store_result(exb, v, v);

  exb.push(char(0x39)); // cmp %eax, %ecx
  exb.push(char(0xc1));

  exb.push(char(0x0f)); // je finish
  exb.push(char(0x84));
  int skip_offset(exb.push_future<unsigned>());

  unsigned offset_count(0);
  for (auto s : schunk[n]) {
    cout << "  successor " << s << endl;
    offset_count += trans_gencall(s);
  }

  exb.push(skip_offset, offset_count);
}

void reg_trans() {
  push_time("reg_trans");

  // Preamble: %edi contains the cycle!
  exb.push(char(0x48)); // mov &now[0], %rbx
  exb.push(char(0xbb));
  exb.push((void*)&now[0]);

  exb.push(char(0x8b)); // mov (%rbx),%edi
  exb.push(char(0x3b));

   for (auto r : rcpy)
    trans_reg(r);

  // For everything not a register in level 0, evaluate.
  for (nodeid_t n = 0; n < nodes.size(); ++n)
    if (ll.count(n) && ll[n] == 0 && !dynamic_cast<regimpl*>(nodes[n]))
      trans_eval(n);

  pop_time();
}

void llbuf_trans() {
  push_time("llbuf_trans");
  
  // Generate the LL buffers themselves
  for (unsigned i = 0; i < ll_size.size(); ++i) {
    ll_base_offset[i] = exb.get_pos();
    // First, generate code for all of the static clusters in this level.
    for (auto c : static_clusters[i])
      trans_cluster(c, true);
    
    // Place a null pointer after all the other pointers in the ll queue
    exb.push(char(0x48)); // mov &ll_pos[i],%rbx
    exb.push(char(0xbb));
    exb.push((void*)&ll_pos[i]);

    exb.push(char(0x48)); // mov (%rbx),%rax
    exb.push(char(0x8b));
    exb.push(char(0x03));
    
    exb.push(char(0x48)); // movq $0,(%rax)
    exb.push(char(0xc7));
    exb.push(char(0x00));
    exb.push(unsigned(0));
    
    // Reset ll_pos[i] to ll_buf[i]
    exb.push(char(0x48)); // mov &ll_buf[i],%rax
    exb.push(char(0xb8));
    exb.push((void*)&ll_buf[i]);

    exb.push(char(0x48)); // mov (%rax),%rax
    exb.push(char(0x8b));
    exb.push(char(0x00));
    
    exb.push(char(0x48)); // mov %rax,(%rbx)
    exb.push(char(0x89));
    exb.push(char(0x03));

    // Call all of the items in the queue
    exb.push(char(0x48)); // top: mov (%rax),%rbx
    exb.push(char(0x8b));
    exb.push(char(0x18));

    exb.push(char(0x48)); //      test %rbx,%rbx
    exb.push(char(0x85));
    exb.push(char(0xdb));
    
    exb.push(char(0x74)); //      jz finish
    exb.push(char(0x0a));

    exb.push(char(0x50)); //      push %rax
  
    exb.push(char(0xff)); //      call *%rbx
    exb.push(char(0xd3));

    exb.push(char(0x58)); //      pop %rax
    
    exb.push(char(0x48)); //      add $8,%rax
    exb.push(char(0x83));
    exb.push(char(0xc0));
    exb.push(char(0x08));
    
    exb.push(char(0xeb)); //      jmp top
    exb.push(char(0xee));

    ll_offset[i] = exb.get_pos(); 
  }

  exb.push(char(0xc3));

  pop_time();
}

void log_trans() {
  push_time("log_trans");

  for (auto &c : clusters)
    trans_cluster(c.first);

  pop_time();
}

void find_static_clusters() {
  push_time("find_static_clusters");
  const unsigned CYC_THRESHOLD(100), SUC_THRESHOLD(1), VIS_THRESHOLD(3000);

  unsigned count(0);
  for (auto &p : ll) {
    nodeid_t n(p.first);
    int l(p.second);
    unsigned cyc(eval_cyc[n]);
    unsigned suc(schunk[n].size());
    for (auto x : schunk[n]) if (static_clusters[l + 1].count(x)) --suc;
    if (l && cyc != ~0 && now[0] - cyc < CYC_THRESHOLD && cyc != ~0 && suc < SUC_THRESHOLD && visited[n] > VIS_THRESHOLD) {
      if (!static_clusters[l].count(n)) ++count;
      static_clusters[l].insert(n);
    }
  }
  
  visited.clear(); visited.resize(nodes.size());
  
  cout << count << " new static clusters." << endl;
  pop_time();
}

void advance_trans(ostream &vcdout) {
  const unsigned DYN_TRANS_INTERVAL(10000);

  if (now[0] % DYN_TRANS_INTERVAL == 0) {
    if (now[0]) pop_time();

    // Don't fix up anything we've already fixed.
    fixup.clear();
    ll_fixup.clear();
    
    // Find clusters to implement static
    find_static_clusters();
    
    // Clear the execbuf.
    exb.clear();

    // Translate the registers.
    reg_trans();

    // Add the logic layer buffers
    llbuf_trans();

    // Translate the logic.
    log_trans();

    // We're through with the code generation. Initialize ll_buf values.
    ll_bufs.resize(ll_size.size() + 1);
    for (unsigned i = 0; i < ll_size.size(); ++i) {
      ll_bufs[i].resize(ll_size[i] + 1);
      ll_buf[i] = ll_pos[i] = (char*)&ll_bufs[i][0];
    }

    // Fix up fixups
    for (auto &f : fixup) {
      if (!implptr.count(f.first)) {
        cout << "ERROR: cluster " << f.first << " not in implptr table" << endl;
        abort();
      }

      for (auto p : f.second)
        exb.push(p, (void*)(implptr[f.first] + exb.buf));
    }

    for (auto &f : ll_fixup) {
      for (auto p : f.second)
        exb.push(p, unsigned(ll_base_offset[f.first] - p - 4));
    }

    // Clear the eval cycles. We want everything to re-evaluate since some
    // static nodes may have become dynamic.
    for (auto &t : eval_cyc) t = ~0;

    push_time("sim");
  }

  for (auto t : trans_tickables) t->tock(e);

  #ifndef NO_VCD 
  print_taps(vcdout, e);
  #endif

  // Run the execbuf; one cycle of evaluation.
  exb();
  
  #if 0
  cout << "Cyc " << now[0] << endl;
  for (unsigned i = 0; i < v.size(); ++i) cout << ' ' << v[i];
  cout << endl;
  for (unsigned i = 0; i < ll_buf.size(); ++i)
    for (unsigned j = 0; *(unsigned long *)&ll_buf[i][j*8]; ++j)
      cout << "ll_buf[" << i << "][" << j << "] = 0x" << hex << *(unsigned long *)&ll_buf[i][j*8] << dec << endl << ';';
  #endif

  for (auto t : trans_tickables) t->tick(e);

  ++now[0];
}

void chdl::run_trans(std::ostream &vcdout, bool &stop, cycle_t max) {
  init_trans();

  print_vcd_header(vcdout);
  for (unsigned i = 0; i < max; ++i) {
    #ifndef NO_VCD
    print_time(vcdout);
    #endif
    advance_trans(vcdout);
  }
  print_time(vcdout);

  pop_time();

  call_final_funcs();

  cout << "Stopwatch values:" << endl;
  for (auto &p : times)
    cout << "  " << p.first << ": " << p.second << "ms" << endl;

  #if FINAL_REPORT
  cout << now[0] << ':';
  for (unsigned i = 0; i < nodes.size(); ++i) cout << ' ' << v[i];
  cout << endl;
  cout << "visited: ";
  for (unsigned i = 0; i < nodes.size(); ++i) cout << ' ' << visited[i];
  cout << endl;
  cout << "last cycle: ";
  for (unsigned i = 0; i < nodes.size(); ++i) cout << ' ' << eval_cyc[i];
  cout << endl;
  #endif


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
