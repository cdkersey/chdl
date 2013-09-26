#include "tristate.h"
#include "trisimpl.h"

#include <vector>

#include <iostream>

using namespace chdl;
using namespace std;

tristatenode::tristatenode(): node((new tristateimpl())->id) {}

void tristatenode::connect(node in, node enable) {
  tristateimpl *t(dynamic_cast<tristateimpl*>(nodes[idx]));
  if (!t) abort(); // assert(t);
  t->connect(in, enable);  
}
