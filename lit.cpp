#include "lit.h"
#include "node.h"
#include "litimpl.h"

using namespace chdl;
using namespace std;

node chdl::Lit(bool val) { return (new litimpl(val))->id; }
