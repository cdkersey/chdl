// The functions defined in this header are used to do simple analyses on the
// design, such as critical path analysis and gate counts.
#include <iostream>

#include <string>
#include <vector>
#include <stack>
#include <ostream>

#include "hierarchy.h"

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
    for (unsigned i = 0; i < level; ++i) out << ' ';
    out << name << endl;

    // Print children
    for (unsigned i = 0; i < c.size(); ++i)
      c[i].print(out, maxlevel, level+1);
  }

  hpath_t path;
  string name;
  vector<hierarchy> c;
};

static hierarchy root("chdl_root");
static stack<hierarchy*> hstack;

void chdl::hierarchy_enter(string name) {
  if (hstack.empty()) hstack.push(&root);

  hstack.top()->new_child(name);
  hstack.push(&(hstack.top()->c[hstack.top()->c.size() - 1]));
}

void chdl::hierarchy_exit() {
  hstack.pop();
}

void chdl::print_hierarchy(ostream &out, int maxlevel) {
  root.print(out, maxlevel);
}

hpath_t chdl::get_hpath() { return hstack.top()->path; }
