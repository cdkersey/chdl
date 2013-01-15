// Operator overloades for the basic gate types.
#ifndef __GATEOPS_H
#define __GATEOPS_H

#include "gates.h"

namespace chdl {
  node operator&&(node a, node b) { return And(a, b);      }
  node operator||(node a, node b) { return Or (a, b);      }
  node operator==(node a, node b) { return Inv(Xor(a, b)); }
  node operator!=(node a, node b) { return Xor(a, b);      }
  node operator! (node i)         { return Inv(i);         }
};

#endif
