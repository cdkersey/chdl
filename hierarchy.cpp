// The functions defined in this header are used to do simple analyses on the
// design, such as critical path analysis and gate counts.
#include <iostream>

#include <string>
#include <vector>
#include <stack>

#include "hierarchy.h"

using namespace std;
using namespace chdl;

struct hierarchy {
  hierarchy(string name): name(name) {}

  void new_child(string n) { c.push_back(hierarchy(n)); }

  void print(unsigned level=0) {
    // Print this
    for (unsigned i = 0; i < level; ++i) cout << ' ';
    cout << name << endl;

    // Print children
    for (unsigned i = 0; i < c.size(); ++i)
      c[i].print(level+1);
  }

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

void chdl::print_hierarchy() {
  root.print();
}
