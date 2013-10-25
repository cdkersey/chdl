// The functions defined in this header are used to do simple analyses on the
// design, such as critical path analysis and gate counts.
#include <iostream>

#include <string>
#include <vector>
#include <stack>
#include <ostream>
#include <map>
#include <set>

#include "hierarchy.h"
#include "nodeimpl.h"
#include "regimpl.h"
#include "reset.h"

using namespace std;
using namespace chdl;

struct hierarchy {
  hierarchy(string name, hpath_t path = hpath_t()): name(name), path(path) {}

  void new_child(string n) {
    hpath_t cpath(path);
    cpath.push_back(c.size());
    c.push_back(hierarchy(n, cpath));
  }

  void print(ostream &out, int maxlevel, unsigned level=0) {
    if (maxlevel > 0 && level > maxlevel) return;

    // Print this
    if (refcount != 0) {
      for (unsigned i = 0; i < level; ++i) out << ' ';
      out << name << '-' << number << " (" << refcount << ')' << endl;
    }

    // Print children
    for (unsigned i = 0; i < c.size(); ++i)
      c[i].print(out, maxlevel, level+1);
  }

  void clear_refcount() {
    void number_instances(hierarchy &);
    number_instances(*this);
    refcount = 0;

    for (unsigned i = 0; i < c.size(); ++i)
      c[i].clear_refcount();
  }

  unsigned number;
  unsigned refcount;
  hpath_t path;
  string name;
  vector<hierarchy> c;
};

static hierarchy root("chdl_root");
static stack<hierarchy*> hstack;

static void reset_hierarchy() {
  while (!hstack.empty()) hstack.pop();
  root = hierarchy("chdl_root");
}
CHDL_REGISTER_RESET(reset_hierarchy);

void number_instances(hierarchy &r) {
  map<string, unsigned> count;

  // Number the instances of different child names of r.
  for (unsigned i = 0; i < r.c.size(); ++i)
    r.c[i].number = count[r.c[i].name]++;
}

void inc_path(hpath_t &path, hierarchy &root, unsigned offset=0)
{
  root.refcount++;

  if (path.size() == offset) return;

  inc_path(path, root.c[path[offset]], offset+1);
}

static void count_refs() {
  root.clear_refcount();

  for (size_t i = 0; i < nodes.size(); ++i)
    inc_path(nodes[i]->path, root);
}

void chdl::hierarchy_enter(string name) {
  if (hstack.empty()) hstack.push(&root);

  hstack.top()->new_child(name);
  hstack.push(&(hstack.top()->c[hstack.top()->c.size() - 1]));
}

void chdl::hierarchy_exit() {
  hstack.pop();
}

void chdl::print_hierarchy(ostream &out, int maxlevel) {
  count_refs();
  root.print(out, maxlevel);
}

hpath_t chdl::get_hpath() {
  if (hstack.empty()) return hpath_t();
  return hstack.top()->path;
}

bool first_elements_eq(const hpath_t &f, const hpath_t &c) {
  if (c.size() <= f.size()) return false;

  for (unsigned i = 0; i < f.size(); ++i)
    if (f[i] != c[i]) return false;
  return true;
}

bool all_elemnts_eq(const hpath_t &a, const hpath_t &b) {
  if (a.size() != b.size()) return false;
  for (unsigned i = 0; i < a.size(); ++i)
    if (a[i] != b[i]) return false;
  return true;
}

hierarchy &get_by_path(hpath_t path) {
  hierarchy *p = &root;
  for (unsigned i = 0; i < path.size(); ++i)
    p = &(p->c[path[i]]);
  return *p;
}

string chdl::path_str(const hpath_t &path) {
  string path_str;
  hierarchy *p = &root; 
  for (unsigned i = 0; i < path.size(); ++i) {
    path_str += "/";
    p = &(p->c[path[i]]);
    path_str += p->name;
  }
  return path_str;
}

void chdl::dot_schematic(std::ostream &out, hpath_t path) {
  count_refs();

  hierarchy &r(get_by_path(path));
  number_instances(r);

  map<unsigned, set<nodeid_t>> incoming_edges;
  map<pair<unsigned, unsigned>, set<nodeid_t>> internal_edges;

  for (size_t i = 0; i < nodes.size(); ++i) {
    if (!first_elements_eq(r.path, nodes[i]->path)) continue;
    unsigned child_idx(nodes[i]->path[r.path.size()]);

    regimpl* rp = dynamic_cast<regimpl*>(nodes[i]);
    if (rp) { 
      // TODO: un-copypaste this chunk of code and clean it up.
      nodeid_t s(rp->d);
      if (first_elements_eq(r.path, nodes[s]->path)) {
        if (nodes[s]->path[r.path.size()] == child_idx) continue;
        pair<unsigned, unsigned>
          e(make_pair(nodes[s]->path[r.path.size()], child_idx));
        internal_edges[e].insert(s);
      } else {
        incoming_edges[child_idx].insert(s);
      }
    }

    for (size_t j = 0; j < nodes[i]->src.size(); ++j) {
      nodeid_t s(nodes[i]->src[j]);
      if (first_elements_eq(r.path, nodes[s]->path)) {
        if (nodes[s]->path[r.path.size()] == child_idx) continue;
        pair<unsigned, unsigned>
          e(make_pair(nodes[s]->path[r.path.size()], child_idx));
        internal_edges[e].insert(s);
      } else {
        incoming_edges[child_idx].insert(s);
      }
    }
  }

  out << "digraph G {" << endl;
  for (unsigned i = 0; i < r.c.size(); ++i) {
    if (r.c[i].refcount == 0) continue;
    out << ' ' << r.c[i].name << '_' << r.c[i].number << " [shape=square];"
        << endl;
  }

  for (auto e : incoming_edges) {
    hierarchy &h(r.c[e.first]);
    out << ' ' << h.name << '_' << h.number
        << "_inputs [shape=none,label=\"\"];" << endl;
    out << ' ' << h.name << '_' << h.number << "_inputs -> " << h.name << '_'
        << h.number << " [label=" << e.second.size() << "];" << endl;
  }

  for (auto e : internal_edges) {
    hierarchy &a(r.c[e.first.first]), &b(r.c[e.first.second]);
    out << ' ' << a.name << '_' << a.number
        << " -> " << b.name << '_' << b.number
        << " [label=" << e.second.size() << "];" << endl;
  }
  out << '}' << endl;
}
