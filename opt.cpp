// This is the CHDL programmer facing side of the optimization layer. It is used
// solely for the purpose of invoking the optimization layer.
#include "opt.h"
#include "optimpl.h"
#include "tap.h"

#include <set>
#include <map>

using namespace chdl;
using namespace std;

void chdl::opt_dead_node_elimination() {
  set<nodeid_t> live_nodes;

  // Start with an initial set of live nodes: taps and register D and Q nodes
  get_tap_nodes(live_nodes);
  get_reg_nodes(live_nodes);
  
  size_t prev_count, count(live_nodes.size());
  do {
    prev_count = count;

    // Mark all of the source nodes for each already-marked node
    for (auto it = live_nodes.begin(); it != live_nodes.end(); ++it) {
      vector<node> &s(nodes[*it]->src);
      for (unsigned i = 0; i < s.size(); ++i) live_nodes.insert(s[i]);
    }

    count = live_nodes.size();
  } while(prev_count != count);

  cerr << "Found " << live_nodes.size() << " live nodes out of "
       << nodes.size()<< " total nodes." << endl;

  // Create the permutation map and permute the nodes.
  map<nodeid_t, nodeid_t> pm;
  nodeid_t dest(0);
  for (nodeid_t i = 0; i < nodes.size(); ++i) {
    if (live_nodes.find(i) != live_nodes.end()) pm[i] = dest++;
  }
  permute_nodes(pm);
}

void chdl::optimize() {
  cerr << "Before optimization: " << nodes.size() << endl;
  opt_dead_node_elimination();
  cerr << "After dead node elimination: " << nodes.size() << endl;
}
