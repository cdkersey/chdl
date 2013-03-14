#include "lit.h"
#include "node.h"
#include "litimpl.h"
#include "hierarchy.h"

using namespace chdl;
using namespace std;

node chdl::Lit(bool val) {
  HIERARCHY_ENTER();
  node r((new litimpl(val))->id);
  HIERARCHY_EXIT();
  return r;
}
