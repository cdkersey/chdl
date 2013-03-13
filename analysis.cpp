#include "opt.h"
#include "tap.h"
#include "gates.h"
#include "nodeimpl.h"
#include "gatesimpl.h"
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

    for (auto it = frontier.begin(); it != frontier.end(); ++it)
        for (unsigned i = 0; i < nodes[*it]->src.size(); ++i)
          next_frontier.insert(nodes[*it]->src[i]);

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

  for (auto it = s.begin(); it != s.end(); ++it) {
    set<nodeid_t> v;
    s.insert(*it);
    if (cycdet_internal(0, *it, v, c)) return true;
  }
  
  return false;
}
