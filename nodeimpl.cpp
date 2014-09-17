#include <unordered_map>
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
unordered_multimap<nodeid_t, node*> &node_dir() {
  static auto nd = new unordered_multimap<nodeid_t, node*>();
  return *nd;
}

// Node dir pair
pair<nodeid_t, node*> ndp(nodeid_t i, node *p) {
  return pair<nodeid_t, node*>(i, p);
}

// Node dir erase
void nde(nodeid_t i, node *p) {
  auto r = node_dir().equal_range(i);
  for (auto it = r.first; it != r.second;) {
    if (it->second == p)
      it = node_dir().erase(it);
    else
      ++it;
  }
}

nodeimpl &chdl::dummynode() {
  static nodeimpl* dn = nodes[Lit('x')];
  return *dn;
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

node::node():              idx(/*nodes.size()*/Lit(0)) { /*nodes.push_back(&dummynode());*/ node_dir().insert(ndp(idx, this)); }
node::node(nodeid_t i):    idx(i)        { node_dir().insert(ndp(idx, this)); }
node::node(const node &r): idx(r.idx)    { node_dir().insert(ndp(idx, this)); }

node::~node() { nde(idx, this); }

node &node::operator=(const node &r) {
  nodeid_t from(idx), to(r.idx);

  nde(from, this);
  if (from != NO_NODE && from != to) {
    // Move all of the nodes to the new node.
    auto r = node_dir().equal_range(from);
    for (auto it = r.first; it != r.second; ++it)
    {
      node_dir().insert(ndp(to, it->second));
      it->second->idx = to;
    }

    node_dir().erase(from);
  }

  node_dir().insert(ndp(to, this));
  idx = to;

  return *this;
}

void node::change_net(nodeid_t i) {
  nde(idx, this);
  node_dir().insert(ndp(i, this));
  idx = i;
}

void show_node_dir() {
  cout << "Node directory:" << endl;
  for (auto &x : node_dir())
    cout << "  " << x.first << ", " << x.second << endl;
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
      if (nodes[i] != &dummynode()) delete nodes[i];
    }
  }

  nodes = new_nodes;
}

void chdl::get_dead_nodes(std::set<nodeid_t> &s) {
  s.clear();

  for (nodeid_t i = 0; i < nodes.size(); ++i) {
    if (!node_dir().count(i)) s.insert(i);
  }

#if 0
  unsigned long n(0);

  for (auto &x : node_dir()) {
    if (x.second.empty()) {
      s.insert(x.first);
      ++n;
    }
  }
#endif

  cout << "Found " << s.size() << " dead nodes." << endl;
}
