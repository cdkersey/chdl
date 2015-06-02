#include "lit.h"
#include "node.h"
#include "litimpl.h"
#include "hierarchy.h"

using namespace chdl;
using namespace std;

node chdl::Lit(char val) {
  nodeid_t r_id;

  HIERARCHY_ENTER();

  if (val == 0 || val == 1) r_id = (new litimpl(val))->id;
  else if (val == '0' || val == '1') r_id = (new litimpl(val == '1'))->id;
  else if (val == 'x' || val == 'X') r_id = (new litimpl())->id;

  node r(r_id);

  HIERARCHY_EXIT();

  return r;
}
