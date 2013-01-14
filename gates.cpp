// Basic gates.
#include "gates.h"
#include "nodeimpl.h"
#include "gatesimpl.h"

using namespace chdl;

node chdl::Nand(node a, node b) { return (new nandimpl(a, b))->id; }
node chdl::Inv (node in)        { return (new invimpl (in))->id;   }
