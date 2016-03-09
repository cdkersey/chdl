#ifndef __NETLIST_H
#define __NETLIST_H

#include <iostream>

namespace chdl {
  void print_verilog(
    const char* module_name, std::ostream &out, bool reset = false
  );

  void print_netlist(std::ostream &out);
};

#endif
