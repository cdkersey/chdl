#ifndef CHDL_CONSOLE_H
#define CHDL_CONSOLE_H

#include <iostream>

#include "ingress.h"
#include "egress.h"

namespace chdl {
  void ConsoleOut(node valid, bvec<8> x, std::ostream &out = std::cout);

  void ConsoleIn(const node &valid, const bvec<8> &x,
                 std::istream &in = std::cin);
}
#endif
