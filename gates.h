// Basic gates.
#ifndef __GATES_H
#define __GATES_H

#include "node.h"

namespace chdl {
  // Functions to instantiate the basic logic types.
  node Nand(node a, node b);
  node Inv (node in);

  // Simple combinations of the basic logic types.
  static node Nor(node a, node b) { return Inv(Nand(Inv(a), Inv(b))); }
  static node And(node a, node b) { return Inv(Nand(a, b));           }
  static node Or (node a, node b) { return Nand(Inv(a), Inv(b));      }
  static node Xor(node a, node b) { return And(Or(a, b), Nand(a, b)); }

  static node Mux(node s, node i0, node i1) {
    return Nand(Nand(Inv(s), i0), Nand(s, i1));
  }
};
#endif
