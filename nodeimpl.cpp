#include <map>
#include <set>

#include "nodeimpl.h"
#include "litimpl.h"

using namespace std;
using namespace chdl;

vector<nodeimpl*> chdl::nodes;

// The node directory keeps track of every node. This is used by optimizations
// to perform transforms requiring changing, e.g. node IDs.
map<nodeid_t, set<node*>> node_dir;

bool litimpl::eval() { return val; }

void litimpl::print(ostream &out) {
  out << "  lit" << val << ' ' << id << endl;
}

node::node():              idx(NO_NODE) { node_dir[idx].insert(this); }
node::node(nodeid_t i):    idx(i)       { node_dir[idx].insert(this); }
node::node(const node &r): idx(r.idx)   { node_dir[idx].insert(this); }

node::~node() { node_dir[idx].erase(this); }

node &node::operator=(const node &r) {
  nodeid_t from(idx), to(r.idx);
  idx = to;

  node_dir[from].erase(this);
  if (from != NO_NODE) {
    // Move all of the nodes to the new node.
    for (auto it = node_dir[from].begin(); it != node_dir[from].end(); ++it) {
      if (from != to) {
        node_dir[to].insert(*it);
        node_dir[from].erase(it);
        (*it)->idx = to;
      }
    }
  }

  node_dir[to].insert(this);
  idx = to;
}

void show_node_dir() {
  cout << "Node directory:" << endl;
  for (auto it = node_dir.begin(); it != node_dir.end(); ++it) {
    cout << it->first << endl;
    for (auto jt = it->second.begin(); jt != it->second.end(); ++jt) {
      cout << "  " << *jt << ' ';
    }
    cout << endl;
  }
}

void chdl::permute_nodes(map<nodeid_t, nodeid_t> x) {
  size_t new_nodes_sz=0;
  for (auto i = x.begin(); i != x.end(); ++i)
    if (i->second + 1 > new_nodes_sz) new_nodes_sz = i->second + 1;

  vector<nodeimpl *> new_nodes(new_nodes_sz);
  for (size_t i = 0; i < nodes.size(); ++i) {
    node n(i);
    if (x.find(i) != x.end()) {
      // This assignment, because of the way nodes work, causes all nodes with
      // index i to point to the new index.
      n = node(x[i]);
      new_nodes[x[i]] = nodes[i];

      // Set the ID.
      new_nodes[x[i]]->id = x[i];
    } else {
      // It's not in the mapping; the nodeimpl can be freed, and the
      // corresponding node objects made to point at NO_NODE.
      n = node(NO_NODE);
      delete nodes[i];
    }
  }

  nodes = new_nodes;
}
