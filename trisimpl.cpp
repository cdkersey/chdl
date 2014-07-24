#include "node.h"
#include "nodeimpl.h"
#include "trisimpl.h"

#include <vector>
#include <iostream>
#include <cstdlib>

using namespace chdl;
using namespace std;

void tristateimpl::connect(node in, node enable) {
  src.push_back(in);
  src.push_back(enable);
  // assert(src.size() % 2 == 0);
}

bool tristateimpl::eval(cdomain_handle_t cd) {
  unsigned nDriven(0);
  bool rval(true);
  for (unsigned i = 0; i < src.size(); i += 2) {
    nodeimpl *pi(nodes[src[i]]), *pe(nodes[src[i+1]]);
    if (pe->eval(cd)) { ++nDriven; rval = pi->eval(cd); }
  }
  // assert(nDriven <= 1);
  if (nDriven > 1) std::abort(); // A tri-state node must have exactly 1 driver
  return rval;
}

void tristateimpl::print(ostream &os) {
  os << "  tri";
  for (unsigned i = 0; i < src.size(); ++i) os << ' ' << src[i];
  os << ' ' << id << endl;
}

void tristateimpl::print_vl(ostream &os) {
  for (unsigned i = 0; i < src.size(); i += 2) {
    os << "  bufif1 __t" << id << '_' << i/2 << "(__x" << id << ", "
       << "__x" << src[i] << ", __x" << src[i+1] << ");" << endl;
  }
}
