#include "lit.h"
#include "node.h"
#include "litimpl.h"
#include "hierarchy.h"

using namespace chdl;
using namespace std;

node chdl::Lit(char val) {
  if (val == 0 || val == 1) {
    HIERARCHY_ENTER();
    node r((new litimpl(val))->id);
    HIERARCHY_EXIT();
    return r;
  } else if (val == '0' || val == '1') {
    HIERARCHY_ENTER();
    node r((new litimpl(val == '1'))->id);
    HIERARCHY_EXIT();
    return r;
  } else if (val == 'x' || val == 'X') {
    HIERARCHY_ENTER();
    node r((new litimpl())->id);
    HIERARCHY_EXIT();
    return r;
  }    
}
