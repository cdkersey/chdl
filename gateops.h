// Operator overloads for the basic gate types.
#ifndef __GATEOPS_H
#define __GATEOPS_H

#include "gates.h"

namespace chdl {
  static node operator&&(node a, node b) { return And(a, b);      }
  static node operator||(node a, node b) { return Or (a, b);      }
  static node operator==(node a, node b) { return Inv(Xor(a, b)); }
  static node operator!=(node a, node b) { return Xor(a, b);      }
  static node operator! (node i)         { return Inv(i);         }
};

#endif
