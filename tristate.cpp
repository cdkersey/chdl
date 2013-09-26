#include "node.h"
#include "nodeimpl.h"
#include "tristate.h"

#include <vector>

#include <iostream>

using namespace chdl;
using namespace std;

class tristateimpl : public nodeimpl {
 public:
  void connect(node in, node enable) {
    src.push_back(in);
    src.push_back(enable);
    // assert(src.size() % 2 == 0); // Even number of input nodes
  }

  bool eval() {
    unsigned nDriven(0);
    bool rval;
    for (unsigned i = 0; i < src.size(); i += 2) {
      nodeimpl *pi(nodes[src[i]]), *pe(nodes[src[i+1]]);
      if (pe->eval()) { ++nDriven; rval = pi->eval(); }
    }
    // assert(nDriven == 1);
    if (nDriven != 1) abort(); // A tri-state node must have exactly 1 driver
    return rval;
  }

  void print(ostream &os) {}
  void print_vl(ostream &os) {}
};

tristatenode::tristatenode(): node((new tristateimpl())->id) {}

void tristatenode::connect(node in, node enable) {
  tristateimpl *t(dynamic_cast<tristateimpl*>(nodes[idx]));
  if (!t) abort(); // assert(t);
  t->connect(in, enable);  
}
