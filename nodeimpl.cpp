#include <unordered_map>
#include <map>
#include <set>

#include <iostream>

#include "nodeimpl.h"
#include "litimpl.h"
#include "lit.h"
#include "reset.h"
#include "cdomain.h"

using namespace std;
using namespace chdl;

vector<nodeimpl*> chdl::nodes;

typedef unordered_map<nodeid_t, set<node*> > node_dir_t;

// The node directory keeps track of every node. This is used by optimizations
// to perform transforms requiring changing, e.g. node IDs.
node_dir_t &node_dir() {
  static auto nd = new unordered_map<nodeid_t, set<node*> >();
  return *nd;
}

// Node dir pair
pair<nodeid_t, node*> ndp(nodeid_t i, node *p) {
  return pair<nodeid_t, node*>(i, p);
}

// Node dir erase
void nde(nodeid_t i, node *p) {
  node_dir()[i].erase(p);
  if (node_dir()[i].empty()) node_dir().erase(i);
}

nodeid_t chdl::nodecount() { return nodes.size() - 1; }

static void clear_nodes() {
  node_dir().clear();
  nodes.clear();
}
CHDL_REGISTER_RESET(clear_nodes);

void nodeimpl::print(ostream &out, print_lang l, print_phase p) {
  if ((p == 10) && (l == PRINT_LANG_VERILOG))
    out << "  wire __x" << id << ';' << endl;
}

bool litimpl::eval(cdomain_handle_t) { return val; }

void litimpl::print(ostream &out) {
  if (undef)  out << "  litX " << id << endl;
  else        out << "  lit" << val << ' ' << id << endl;
}

void litimpl::print_vl(ostream &out) {
  out << "  assign __x" << id << " = " << val << ';' << endl;
}

void litimpl::print(std::ostream &out, print_lang l, print_phase p) {
  nodeimpl::print(out, l, p);
  
  if (p = 100) {
    if (l == PRINT_LANG_NETLIST) {
      if (undef) out << "  litX " << id << endl;
      else       out << "  lit" << val << ' ' << id << endl;
    } else if (l == PRINT_LANG_VERILOG) {
      out << "  assign __x" << id << " = " << val << ';' << endl;
    }
  }
}

node::node(): idx(nodes.size()) {
  new litimpl();
  check();
  node_dir()[idx].insert(this);
}

node::node(nodeid_t i):    idx(i)    { check(); node_dir()[idx].insert(this); }
node::node(const node &r): idx(r.idx){ check(); node_dir()[idx].insert(this); }

node::~node() { nde(idx, this); }

const node &node::operator=(const node &r) const {
  nodeid_t from(idx), to(r.idx);

  if (from != NO_NODE && from != to) {
    for (auto x : node_dir()[from]) {
      node_dir()[to].insert(x);
      x->idx = to;
    }

    node_dir().erase(from);
  }

  check();

  return *this;
}

node &node::operator=(const node &r) {
  *(const node *)this = r;
  return *this;
}

void node::change_net(nodeid_t i) {
  nde(idx, this);
  node_dir()[i].insert(this);
  idx = i;
  check();
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

void chdl::get_dead_nodes(std::set<nodeid_t> &s) {
  s.clear();

  for (nodeid_t i = 0; i < nodes.size(); ++i)
    if (!node_dir().count(i) || node_dir()[i].size() == 0) s.insert(i);

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

void chdl::nodeimpl::predecessors
  (print_lang l, print_phase p, set<printable *> &s)
{
  s.clear();

  if (p == 100)
    for (auto &p : src)
      s.insert(nodes[(nodeid_t)p]);
}

