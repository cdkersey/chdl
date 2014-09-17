// Basic gates.
#ifndef __GATES_H
#define __GATES_H

#include "node.h"
#include "hierarchy.h"

namespace chdl {
  // Functions to instantiate the basic logic types.
  node Nand(const node &a, const node &b);
  node Inv (const node &in);

  // Simple combinations of the basic logic types.
  static node Nor(const node &a, const node &b) {
    HIERARCHY_ENTER();
    node r(Inv(Nand(Inv(a), Inv(b))));
    HIERARCHY_EXIT();
    return r;
  }

  static node And(const node &a, const node &b) {
    HIERARCHY_ENTER();
    node r(Inv(Nand(a, b)));
    HIERARCHY_EXIT();
    return r;
  }

  static node Or(const node &a, const node &b) {
    HIERARCHY_ENTER();
    node r(Nand(Inv(a), Inv(b)));
    HIERARCHY_EXIT();
    return r;
  }

  static node Xor(const node &a, const node &b) {
    HIERARCHY_ENTER();
    node r(And(Or(a, b), Nand(a, b)));
    HIERARCHY_EXIT();
    return r;
  }

  static node Mux(const node &s, const node &i0, const node &i1) {
    HIERARCHY_ENTER();
    node r(Nand(Nand(Inv(s), i0), Nand(s, i1)));
    HIERARCHY_EXIT();
    return r;
  }

};
#endif
