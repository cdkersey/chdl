// The functions defined in this header are used to do simple analyses on the
// design, such as critical path analysis and gate counts.
#ifndef __HIERARCHY_H
#define __HIERARCHY_H
#include <string>
#include <vector>
#include <ostream>

namespace chdl {
  typedef std::vector<unsigned> hpath_t;

  void hierarchy_enter(std::string name);
  void hierarchy_exit();
  void print_hierarchy(std::ostream &out, int maxlevel=-1);
  void dot_schematic(std::ostream &out, hpath_t path = hpath_t());

  std::string path_str(const hpath_t &path);

  hpath_t get_hpath();
};

#define HIERARCHY_ENTER() chdl::hierarchy_enter(__func__)
#define HIERARCHY_EXIT()  chdl::hierarchy_exit()

#endif
