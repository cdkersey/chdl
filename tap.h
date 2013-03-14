// This interface allows values to be assigned names so that they can be watched
// during simulation.
#ifndef __TAP_H
#define __TAP_H
#include <string>
#include <iostream>
#include <set>

#include "node.h"
#include "bvec.h"

#include "hierarchy.h"

namespace chdl {
  void tap(std::string name, node node);
  template <unsigned N> void tap(std::string name, const bvec<N> &vec);

  void print_tap_nodes(std::ostream &out);
  void print_taps(std::ostream &out);
  void print_vcd_header(std::ostream &out);

  // Append the set of tap nodes to s. Do not clear s.
  void get_tap_nodes(std::set<nodeid_t> &s);
};

template <unsigned N>
  void chdl::tap(std::string name, const bvec<N> &vec)
{
  for (unsigned i = 0; i < N; ++i) tap(name, vec[i]);
}

#define TAP(x) do { tap(#x, x); } while(0)

#endif
