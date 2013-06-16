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
  void tap(std::string name, node node, bool output=false);
  template <unsigned N>
    void tap(std::string name, const bvec<N> &vec, bool output=false);

  void print_taps_vl_head(std::ostream &out);
  void print_taps_vl_body(std::ostream &out, bool print_non_out=false);
  void print_tap_nodes(std::ostream &out);
  void print_taps(std::ostream &out);
  void print_vcd_header(std::ostream &out);
  void print_taps_c_head(std::ostream &out);
  void print_taps_c_body(std::ostream &out);

  // Append the set of tap nodes to s. Do not clear s.
  void get_tap_nodes(std::set<nodeid_t> &s);
};

template <unsigned N>
  void chdl::tap(std::string name, const bvec<N> &vec, bool output)
{
  for (unsigned i = 0; i < N; ++i) tap(name, vec[i], output);
}

#define TAP(x) do { tap(#x, x); } while(0)
#define OUTPUT(x) do { tap(#x, x, true); } while(0)

#endif
