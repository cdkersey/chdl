#include "latch.h"
#include "gates.h"

using namespace chdl;
using namespace std;

node chdl::Latch(node l, node d) {
  HIERARCHY_ENTER();
  node r(Mux(l, d, Wreg(Inv(l), d)));
  HIERARCHY_EXIT();
  return r;
}
