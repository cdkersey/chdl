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
  for (auto g : gates)
    for (auto s : nodes[g]->src)
      if (gates.find(s) == gates.end())
        iset.insert(s);

  for (auto i : iset)
    inputs.push_back(i);
}


vector<gatecluster> chdl::get_gate_clusters(unsigned max_inputs) {
  // Pre-compute successors for each node
  map<nodeid_t, set<nodeid_t>> s;
  for (nodeid_t i = 0; i < nodes.size(); ++i) {
    if (regimpl* r = dynamic_cast<regimpl*>(nodes[i])) {
      s[r->d].insert(i);
    } else {
      for (auto j : nodes[i]->src)
        if (dynamic_cast<nandimpl*>(nodes[i])||dynamic_cast<invimpl*>(nodes[i]))
          s[j].insert(i);
    }
  }

  // Data structures to store clusters
  typedef size_t clusterid_t;
  vector<set<nodeid_t>> n;      // Set of nodes in each cluster
  map<nodeid_t, clusterid_t> c; // Map from node to cluster

  // Start: each cluster contains exactly one gate
  for (nodeid_t i = 0; i < nodes.size(); ++i) {
    if (dynamic_cast<nandimpl*>(nodes[i])||dynamic_cast<invimpl*>(nodes[i])) {
      c[i] = n.size();
      n.resize(n.size() + 1);
      n[c[i]].insert(i);
    }
  }

  // Iterative algorithm: if a cluster's input touches only nodes in that
  // cluster, merge the cluster with the cluster of the gate providing that
  // input.
  bool changed;
  do {
    changed = false;
    for (clusterid_t i = 0; i < n.size(); ++i) {
      for (auto j : n[i]) {
        for (auto k : nodes[j]->src) {
          if (c.find(k) == c.end() || c[k] == i || s[k].empty()) continue;

          if (n[c[k]].find(k) == n[c[k]].end()) {
            cout << "Node " << k << " not found in:";
            for (auto l : n[c[k]]) cout << ' ' << l;
            cout << endl;
            exit(1);
          }

          bool successorsInI(true);
          for (auto l : s[k])
            if (c.find(l) == c.end() || c[l] != i) successorsInI = false;
          if (successorsInI) {
            if (max_inputs) {
              set<nodeid_t> inputs;
              for (auto g : n[c[k]])
                for (auto j : nodes[g]->src)
                  if (c.find(j) == c.end() || (c[j] != c[k] && c[j] != i))
                    inputs.insert(j);
              for (auto g : n[i])
                for (auto j : nodes[g]->src)
                  if (c.find(j) == c.end() || (c[j] != c[k] && c[j] != i))
                    inputs.insert(j);
              if (inputs.size() > max_inputs) continue;
            }

            changed = true;
            clusterid_t oldcluster(c[k]);
            for (auto l : n[c[k]]) {
              n[i].insert(l);
              c[l] = i;
            }
            n[oldcluster].clear();
          }
        }
      }
    }
  } while (changed);

  // Output: Prune out the empty sets
  vector<gatecluster> out;
  for (clusterid_t i = 0; i < n.size(); ++i) {
    if (!n[i].empty()) {
      nodeid_t output;
      for (nodeid_t g : n[i]) {
        if(s.find(g) == s.end()) {
          output = g;
        } else {
          for (nodeid_t j : s[g])
            if (n[i].find(j) == n[i].end())
              output = g;
        }
      }
      out.push_back(gatecluster(output, n[i]));
    }
  }

  return out;
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
