// This is the CHDL programmer facing side of the optimization layer. It is used
// solely for the purpose of invoking the optimization layer.
#ifndef __VIS_H
#define __VIS_H

#include <iostream>

namespace chdl {
  void print_dot(std::ostream &os);
};

#endif
