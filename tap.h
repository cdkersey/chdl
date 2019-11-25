// This interface allows values to be assigned names so that they can be watched
// during simulation.
#ifndef __TAP_H
#define __TAP_H
#include <string>
#include <sstream>
#include <iostream>
#include <set>

#include "node.h"
#include "bvec.h"
#include "bus.h"
#include "tristate.h"
#include "cdomain.h"

#include "hierarchy.h"

namespace chdl {
  void tap(std::string name, tristatenode t, bool output=false);
  void tap(std::string name, node node, bool output=false);
  void change_tap(nodeid_t old_node, nodeid_t new_node);

  void gtap(node node);
  template <typename T, unsigned N>
    void tap(std::string name, const vec<N, T> &vec, bool output=false);
  template <unsigned N>
    void tap(std::string name, const bvec<N> &vec, bool output=false);
  template <unsigned N>
    void tap(std::string name, const bus<N> &vec, bool output=false);
  template <unsigned N> void gtap(const bvec<N> &vec);
  template <unsigned N> void gtap(const bus<N> &vec);

  void print_taps_vl_head(std::ostream &out);
  void print_taps_vl_body(std::ostream &out, bool print_non_out=true);
  void print_tap_nodes(std::ostream &out);
  void print_io_tap_nodes(std::ostream &out);
  void print_taps(std::ostream &out, cdomain_handle_t cd = 0);
  void print_vcd_header(std::ostream &out);

  void get_tap_map(std::map<nodeid_t, std::string> &m);
  void get_tap_map(std::map<std::string, std::vector<nodeid_t> > &m);

  // Append the set of tap nodes to s. Do not clear s.
  void get_tap_nodes(std::set<nodeid_t> &s);
};

template <typename T, unsigned N>
  void chdl::tap(std::string prefix, const vec<N, T> &v, bool output)
{
  for (unsigned i = 0; i < N; ++i) {
    std::ostringstream oss; oss << i;
    tap(prefix + '_' + oss.str(), v[i], output);
  }
}

template <unsigned N>
  void chdl::tap(std::string name, const bvec<N> &vec, bool output)
{
  for (unsigned i = 0; i < N; ++i) tap(name, vec[i], output);
}

template <unsigned N>
  void chdl::tap(std::string name, const bus<N> &vec, bool output)
{
  for (unsigned i = 0; i < N; ++i) tap(name, vec[i], output);
}

template <unsigned N> void chdl::gtap(const chdl::bvec<N> &vec) {
  for (unsigned i = 0; i < N; ++i) gtap(vec[i]);
}

template <unsigned N> void chdl::gtap(const chdl::bus<N> &vec) {
  for (unsigned i = 0; i < N; ++i) gtap(chdl::node(vec[i]));
}

#define TAP(x) do { tap(#x, x); } while(0)
#define OUTPUT(x) do { tap(#x, x, true); } while(0)

#endif
