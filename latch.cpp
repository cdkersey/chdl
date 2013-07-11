#include "latch.h"
#include "gates.h"

using namespace chdl;
using namespace std;

node chdl::Latch(node l, node d) {
  return Mux(l, d, Wreg(Inv(l), d));
}
