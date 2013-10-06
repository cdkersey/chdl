#include "tristate.h"
#include "trisimpl.h"

#include <vector>
#include <cstdlib>

using namespace chdl;
using namespace std;

tristatenode::tristatenode(): node((new tristateimpl())->id) {}

tristatenode::tristatenode(nodeid_t id): node(id) {}

void tristatenode::connect(node in, node enable) {
  tristateimpl *t(dynamic_cast<tristateimpl*>(nodes[idx]));
  if (!t) std::abort(); // assert(t);
  t->connect(in, enable);  
}
