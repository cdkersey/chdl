#include "opt.h"
#include "tap.h"
#include "gates.h"
#include "nodeimpl.h"
#include "gatesimpl.h"
#include "litimpl.h"
#include "netlist.h"
#include "lit.h"
#include "node.h"

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
