// The functions defined in this header are used to do simple analyses on the
// design, such as critical path analysis and gate counts.
#include <iostream>

#include <string>
#include <vector>
#include <stack>
#include <ostream>

#include "hierarchy.h"
#include "nodeimpl.h"

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
      out << name << " (" << refcount << ')' << endl;
    }

    // Print children
    for (unsigned i = 0; i < c.size(); ++i)
      c[i].print(out, maxlevel, level+1);
  }

  void clear_refcount() {
    refcount = 0;

    for (unsigned i = 0; i < c.size(); ++i)
      c[i].clear_refcount();
  }

  unsigned refcount;
  hpath_t path;
  string name;
  vector<hierarchy> c;
};

static hierarchy root("chdl_root");
static stack<hierarchy*> hstack;

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
