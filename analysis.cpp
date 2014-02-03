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
#include <set>
#include <map>
#include <limits>

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

// Emit a report showing the location (in hierarchy) of top 10 longest paths
void chdl::critpath_report(ostream &out) {
  out << "CHDL Critical Path Report" << endl;

  // For each node, find the longest path to it.
  set<nodeid_t> frontier;
  get_tap_nodes(frontier);
  get_reg_nodes(frontier);
  get_mem_nodes(frontier);

  map<nodeid_t, int> longestpath;

  map<nodeid_t, set<nodeid_t> > succ;
  for (nodeid_t n = 0; n < nodes.size(); ++n) {
    for (auto s : nodes[n]->src)
      succ[s].insert(n);

    regimpl *ri(dynamic_cast<regimpl*>(nodes[n]));
    if (ri) succ[ri->d].insert(n);
  }

  while (!frontier.empty()) {
    set<nodeid_t> next_frontier;

    for (auto n : frontier) {
      for (auto p : nodes[n]->src)
        next_frontier.insert(p);

      for (auto s : succ[n])
        if (longestpath[n] <= longestpath[s])
          longestpath[n] = longestpath[s] + 1;
    }

    frontier = next_frontier;
  }

  // We need this map in both directions.
  map<int, set<nodeid_t> > longestpath_r;
  for (auto x : longestpath) longestpath_r[x.second].insert(x.first);

  out << "Top 10 longest paths (in their entirety):" << endl;
  auto it1(longestpath_r.rbegin()); // Major: sets of nodes with pathlength L
  auto it0(it1->second.begin());    // Minor: nodes in set
  for (unsigned i = 0; i < 10 && it1 != longestpath_r.rend(); ++i) {
    out << "  Length " << it1->first - 1 << ':' << endl;

    nodeid_t n(*it0);
    int maxpath, count(it1->first);
    do {
      out << "    " << path_str(nodes[n]->path) << endl;

      maxpath = -1;
      for (auto p : succ[n]) {
        if (longestpath[p] > maxpath) {
          n = p;
          maxpath = longestpath[p];
        }
      }
    } while (--count);
    
    // Go to next node with next longest path length, whether it's at this path
    // length or the next one down.
    if (++it0 == it1->second.end()) {
      ++it1;
      it0 = it1->second.begin();
    }
  }
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
