#include <map>
#include <set>

#include "nodeimpl.h"
#include "litimpl.h"
#include "lit.h"
#include "reset.h"
#include "cdomain.h"

using namespace std;
using namespace chdl;

vector<nodeimpl*> chdl::nodes;

// The node directory keeps track of every node. This is used by optimizations
// to perform transforms requiring changing, e.g. node IDs.
map<nodeid_t, set<node*>> &node_dir() {
  static auto nd = new map<nodeid_t, set<node*>>();
  return *nd;
}

static void clear_nodes() {
  node_dir().clear();
  nodes.clear();
}
CHDL_REGISTER_RESET(clear_nodes);

bool litimpl::eval(cdomain_handle_t cd) { return val; }

void litimpl::print(ostream &out) {
  if (undef)  out << "  litX " << id << endl;
  else        out << "  lit" << val << ' ' << id << endl;
}

void litimpl::print_vl(ostream &out) {
  out << "  assign __x" << id << " = " << val << ';' << endl;
}

node::node():              idx(Lit('x')) { node_dir()[idx].insert(this); }
node::node(nodeid_t i):    idx(i)        { node_dir()[idx].insert(this); }
node::node(const node &r): idx(r.idx)    { node_dir()[idx].insert(this); }

node::~node() { node_dir()[idx].erase(this); }

node &node::operator=(const node &r) {
  nodeid_t from(idx), to(r.idx);

  node_dir()[from].erase(this);
  if (from != NO_NODE && from != to) {
    // Move all of the nodes to the new node.
    for (auto it = node_dir()[from].begin(); it != node_dir()[from].end(); ++it)
    {
      node_dir()[to].insert(*it);
      (*it)->idx = to;
    }

    node_dir()[from].clear();
  }

  node_dir()[to].insert(this);
  idx = to;

  return *this;
}

void node::change_net(nodeid_t i) {
  node_dir()[idx].erase(this);
  node_dir()[i].insert(this);
  idx = i;
}

void show_node_dir() {
  cout << "Node directory:" << endl;
  for (auto n : node_dir()) {
    cout << n.first << endl;
    for (auto m : n.second) {
      cout << "  " << m << ' ';
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
