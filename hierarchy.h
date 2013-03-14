// The functions defined in this header are used to do simple analyses on the
// design, such as critical path analysis and gate counts.
#ifndef __HIERARCHY_H
#define __HIERARCHY_H
#include <string>

namespace chdl {
  void hierarchy_enter(std::string name);
  void hierarchy_exit();
  void print_hierarchy();
};

#define HIERARCHY_ENTER() chdl::hierarchy_enter(__func__)
#define HIERARCHY_EXIT()  chdl::hierarchy_exit()

#endif
