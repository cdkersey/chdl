#include "opt.h"
#include "tap.h"
#include "gates.h"
#include "nodeimpl.h"
#include "gatesimpl.h"
#include "regimpl.h"
#include "litimpl.h"
#include "netlist.h"
#include "lit.h"
#include "node.h"
#include "memory.h"

#include "analysis.h"

#include <vector>
#include <queue>
#include <set>
#include <map>

using namespace chdl;
using namespace std;

unsigned chdl::critpath() {
  set<nodeid_t> frontier;
  get_tap_nodes(frontier);
  get_reg_nodes(frontier);
  get_mem_nodes(frontier);
  
  unsigned l(0);

  while (!frontier.empty()) {
    set<nodeid_t> next_frontier;

    for (auto n : frontier)
        for (unsigned i = 0; i < nodes[n]->src.size(); ++i)
          next_frontier.insert(nodes[n]->src[i]);

    frontier = next_frontier;
    ++l;
  }

  return l;
}

static bool cycdet_internal
  (unsigned lvl, nodeid_t node, set<nodeid_t> &v, set<nodeid_t> &c)
{
  if (c.find(node) != c.end()) return false;
  for (unsigned i = 0; i < nodes[node]->src.size(); ++i) {
    nodeid_t s(nodes[node]->src[i]);
    set<nodeid_t> v2(v);
    v2.insert(s);
    if (v.find(s) != v.end()) return true;
    if (cycdet_internal(lvl+1, s, v2, c)) return true;
  }
  c.insert(node);
  return false;
}

bool chdl::cycdet() {
  set<nodeid_t> s, c;
  get_tap_nodes(s);
  get_reg_nodes(s);
  get_mem_nodes(s);

  for (auto n : s) {
    set<nodeid_t> v;
    if (cycdet_internal(0, n, v, c)) return true;
  }
  
  return false;
}

gatecluster::gatecluster(nodeid_t o, const std::set<nodeid_t> &g) :
  output(o), gates(g)
{
  set<nodeid_t> iset;
  for (auto &g : gates)
    for (auto &s : nodes[g]->src)
      if (gates.find(s) == gates.end())
        iset.insert(s);

  for (auto i : iset)
    inputs.push_back(i);
}

void get_all_delays(map<pair<nodeid_t, nodeid_t>, int> &d) {
  d.clear();

  // dest -> (src -> latency)
  map<nodeid_t, map<nodeid_t, int>> l;

  // The delay from everything to itself is zero
  for (unsigned n = 0; n < nodes.size(); ++n) l[n][n] = 0;

  bool changed(false);
  do {
    changed = false;
    for (unsigned n = 0; n < nodes.size(); ++n) {
      map<nodeid_t, int> &m(l[n]);
      for (auto s : nodes[n]->src) {
        map<nodeid_t, int> &sm(l[s]);
        for (auto p : sm) {
          if (m[p.first] < p.second + 1) {
            changed = true;
            m[p.first] = p.second + 1;
          }
        }
      }
    }
  } while(changed);

  // Populate d from l
  for (auto x : l)
    for (auto y : x.second)
      if (y.second) d[pair<nodeid_t, nodeid_t>(y.first, x.first)] = y.second;
}

void show_delay_matrix(const map<pair<nodeid_t, nodeid_t>, int> &d) {
  for (auto x : d)
    cout << x.first.first << "->" << x.first.second
         << ": " << x.second << endl;
  cout << endl;
}

void get_preds(set<nodeid_t> &p, nodeid_t n) {
  queue<nodeid_t> q;
  for (auto s : nodes[n]->src) q.push(s);

  while (!q.empty()) {
    for (auto s : nodes[q.front()]->src)
      q.push(s);
    p.insert(q.front());
    q.pop();
  }
}

void topo_ordered_nodes(vector<nodeid_t> &v,
                        const map<pair<nodeid_t, nodeid_t>, int> &d)
{
  v.clear();
  map<int, nodeid_t> m;

  for (auto x : d)
    if (x.second > m[x.first.second]) m[x.first.second] = x.second;

  for (auto n : m) {
    v.push_back(n.second);
    cout << n.first << ": " << n.second << endl;
  }
}

size_t n_max_val(map<nodeid_t, int> &s) {
  if (s.empty()) return 0;

  size_t count(0), firstval(s.rbegin()->second);
  for (auto i = s.rbegin(); i != s.rend() && i->second == firstval; ++i)
    ++count;

  return count;
}

void label_nodes(map<nodeid_t, int> &l,
                 map<pair<nodeid_t, nodeid_t>, int> &d, unsigned csize)
{
  l.clear();

  vector<nodeid_t> ordered_nodes;
  topo_ordered_nodes(ordered_nodes, d);

  for (auto n : ordered_nodes) {
    set<nodeid_t> p, cluster;
    get_preds(p, n);

    map<nodeid_t, int> lv;
    for (auto &x : p)
      lv[x] = l[x] + d[pair<nodeid_t, nodeid_t>(x, n)];

    int l1(0), l2(0);
    cluster.insert(n);

    while (!p.empty() && lv.size() + n_max_val(lv) <= csize) {
      cluster.insert(
    }
  }
}

// Rajaraman and Wong clustering
vector<gatecluster> chdl::get_gate_clusters(unsigned max_inputs) {
  // Compute delays from each node to each rechable successor
  map<pair<nodeid_t, nodeid_t>, int> delay;
  get_all_delays(delay);
  show_delay_matrix(delay);

  // Label the nodes
  map<nodeid_t, int> label;
  label_nodes(label, delay, max_inputs);

  return vector<gatecluster>();
}

size_t chdl::num_nands() {
  size_t count(0);
  for (nodeid_t i = 0; i < nodes.size(); ++i)
    if (dynamic_cast<nandimpl*>(nodes[i])) ++count;
  return count;
}

size_t chdl::num_inverters() {
  size_t count(0);
  for (nodeid_t i = 0; i < nodes.size(); ++i)
    if (dynamic_cast<invimpl*>(nodes[i])) ++count;
  return count;  
}

size_t chdl::num_regs() {
  size_t count(0);
  for (nodeid_t i = 0; i < nodes.size(); ++i)
    if (dynamic_cast<regimpl*>(nodes[i])) ++count;
  return count;
}
