#ifndef SUBMODULE_H
#define SUBMODULE_H
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "node.h"
#include "bvec.h"

namespace chdl {
  class module {
  public:
    module(std::string name);

    void print(std::ostream &out);
    void print_vl(std::ostream &out);

    // Functor interface used to attach inputs/outputs
    template <unsigned N> module &operator()(bvec<N> x);
    template <unsigned N> module &inputs(bvec<N> in);
    template <unsigned N> module &outputs(bvec<N> out);

    bool first_output;

  private:
    bool adding_outputs;
    std::string name;
    std::vector<std::vector<node>> i_sigs;
    std::vector<std::vector<node>> o_sigs;
  };  

  void get_module_inputs(std::set<nodeid_t> &s);
  void get_module_outputs(std::set<nodeid_t> &s);

  module &Module(std::string name);
};

template <unsigned N> chdl::module &chdl::module::operator()(chdl::bvec<N> x) {
  if (adding_outputs)
    outputs(x);
  else
    inputs(x);

  return *this;
}

template <unsigned N> chdl::module &chdl::module::inputs(chdl::bvec<N> in) {
  void __chdl_add_module_input(chdl::node n);
  
  adding_outputs = false;

  std::vector<chdl::node> v;
  for (unsigned i = 0; i < N; ++i) {
    __chdl_add_module_input(in[i]);
    v.push_back(in[i]);
  }
  i_sigs.push_back(v);

  return *this;
}

template <unsigned N> chdl::module &chdl::module::outputs(chdl::bvec<N> out) {
  void __chdl_add_module_output(chdl::module &m, chdl::node n);

  adding_outputs = true;

  std::vector<chdl::node> v;
  for (unsigned i = 0; i < N; ++i) {
    __chdl_add_module_output(*this, out[i]);
    v.push_back(out[i]);
  }
  o_sigs.push_back(v);

  return *this;
}

#endif
