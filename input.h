#ifndef __INPUT_H
#define __INPUT_H
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "node.h"
#include "bvec.h"

namespace chdl {
  node Input(std::string name);
  template <unsigned N> bvec<N> Input(std::string name);

  void print_inputs_vl_head(std::ostream &out);
  void print_inputs_vl_body(std::ostream &out);
  void print_input_nodes(std::ostream &out);

  // Append input node IDs to the given set.
  void get_input_nodes(std::set<nodeid_t> &s);

  void get_input_map(std::map<std::string, std::vector<nodeid_t> > &m);

  std::vector<node> input_internal(std::string name, unsigned n);
};

template<unsigned N> chdl::bvec<N> chdl::Input(std::string name) {
  using namespace chdl;
  using namespace std;

  vector<node> v = input_internal(name, N);

  bvec<N> out;
  for (unsigned i = 0; i < N; ++i) out[i] = v[i];
  return out;
}

#endif
