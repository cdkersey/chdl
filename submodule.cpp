#include "submodule.h"
#include "nodeimpl.h"
#include "cdomain.h"

using namespace std;
using namespace chdl;

static vector<node> module_inputs, module_outputs;

class mnodeimpl : public nodeimpl {
  public:
    mnodeimpl(module &m): nodeimpl(), m(m), first(m.first_output)
      { m.first_output = false; }
    
    bool eval(evaluator_t &e) {
      cerr << "Attempted to simulate a design containing submodules." << endl;
      abort();
    }

    void print(ostream &out) { if (first) m.print(out); }
    void print_vl(ostream &out) { if (first) m.print_vl(out); }

  private:
    bool first;
    module &m;
};

namespace chdl {
  void __chdl_add_module_input(node n) {
    module_inputs.push_back(n);
  }

  void __chdl_add_module_output(module &m, node n) {
    module_outputs.push_back(n);
    n = node((new mnodeimpl(m))->id);
  }
}

chdl::module::module(string name): name(name), first_output(true) {}

void chdl::module::print(std::ostream &out) {
  out << "  module<" << name << ">";
  for (unsigned i = 0; i < i_sigs.size(); ++i)
    for (unsigned j = 0; j < i_sigs[i].size(); ++j)
      out << ' ' << i_sigs[i][j];
  for (unsigned i = 0; i < o_sigs.size(); ++i)
    for (unsigned j = 0; j < o_sigs[i].size(); ++j)
      out << ' ' << o_sigs[i][j];
  out << endl;
}

void chdl::module::print_vl(std::ostream &out) {
  static unsigned seq(0);
  out << "  " << name << ' ' << name << seq << "(phi";
  for (unsigned i = 0; i < i_sigs.size(); ++i) {
    if (i_sigs[i].size() > 1) {
      out << ", {__x" << i_sigs[i][i_sigs[i].size()-1];
      for (int j = i_sigs[i].size()-2; j >= 0; --j)
        out << ", __x" << i_sigs[i][j];
      out << '}';
    } else {
      out << ", __x" << i_sigs[i][0];
    }
  }
  for (unsigned i = 0; i < o_sigs.size(); ++i) {
    if (o_sigs[i].size() > 1) {
      out << ", {__x" << o_sigs[i][o_sigs[i].size()-1];
      for (int j = o_sigs[i].size()-2; j >= 0; --j)
        out << ", __x" << o_sigs[i][j];
      out << '}';
    } else {
      out << ", __x" << o_sigs[i][0];
    }
  }
  out << ");" << endl;
  ++seq;
}

void chdl::get_module_inputs(set<nodeid_t> &s) {
  for (unsigned i = 0; i < module_inputs.size(); ++i)
    s.insert(module_inputs[i]);
}

void chdl::get_module_outputs(set<nodeid_t> &s) {
  for (unsigned i = 0; i < module_outputs.size(); ++i)
    s.insert(module_outputs[i]);
}

module &chdl::Module(string name) {
  return *(new module(name));
}
