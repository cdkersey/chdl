// This interface allows values to be assigned names so that they can be watched
// during simulation.
#ifndef __TAP_H
#define __TAP_H
#include <string>
#include <iostream>
#include <set>

#include "node.h"
#include "bvec.h"

namespace chdl {
  void tap(std::string name, node node);
  template <unsigned N, template <unsigned> class T>
    void tap(std::string name, const T<N> &vec);

  void print_tap_nodes(std::ostream &out);
  void print_taps(std::ostream &out);
  void print_vcd_header(std::ostream &out);

  // Append the set of tap nodes to s. Do not clear s.
  void get_tap_nodes(std::set<nodeid_t> &s);
};

template <unsigned N, template <unsigned> class T>
  void chdl::tap(std::string name, const T<N> &vec)
{
  for (unsigned i = 0; i < N; ++i) tap(name, vec[i]);
}

#define TAP(x) do { tap(#x, x); } while(0)

#endif
