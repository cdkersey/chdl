// Basic gates.
#include "gates.h"
#include "nodeimpl.h"
#include "gatesimpl.h"
#include "hierarchy.h"

using namespace chdl;

node chdl::Nand(node a, node b) {
  HIERARCHY_ENTER();
  node r((new nandimpl(a, b))->id);
  HIERARCHY_EXIT();
  return r;
}

node chdl::Inv (node in) {
  HIERARCHY_ENTER();
  node r((new invimpl (in))->id);
  HIERARCHY_EXIT();
  return r;
}
