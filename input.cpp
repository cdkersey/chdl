#include <cstdlib>
#include <iostream>
#include <string>
#include <map>
#include <vector>

#include "input.h"
#include "nodeimpl.h"

using namespace std;
using namespace chdl;

map <string, vector<node>> inputs;

class inputimpl : public nodeimpl {
  public:
    inputimpl(string n, int i=-1): nodeimpl(), name(n), pos(i) {}
    
    bool eval() {
      cerr << "Attempted to simulate a design with unassociated inputs."
           << endl;
      abort();
    }

    void print(ostream &out) {}
    void print_vl(ostream &out) {}

  private:
    // Name of input in input map
    string name;
    // Position in vector in input map. -1 for no name
    int pos;
};

node chdl::Input(std::string name) {
  inputimpl *n = new inputimpl(name, -1);
  inputs[name] = vector<node>(1, n->id);
  return node(n->id);
}

void chdl::print_inputs_vl_head(std::ostream &out) {
  for (auto it = inputs.begin(); it != inputs.end(); ++it)
    out << ',' << endl << "  " << it->first;
}

void chdl::print_inputs_vl_body(std::ostream &out) {
  for (auto it = inputs.begin(); it != inputs.end(); ++it) {
    out << "  input ";
    if (it->second.size() > 1)
      out << '[' << it->second.size()-1 << ":0] ";
    out << it->first << ';' << endl;
    if (it->second.size() > 1) {
      for (unsigned i = 0; i < it->second.size(); ++i) {
        out << "  assign __x" << it->second[i] << " = "
            << it->first << '[' << i << "];" << endl;
      }
    } else {
      out << "  assign __x" << it->second[0] << " = "
          << it->first << ';' << endl;
    }
  }
}

void chdl::print_input_nodes(std::ostream &out) {
  for (auto it = inputs.begin(); it != inputs.end(); ++it) {
    out << "  " << it->first;
    for (unsigned i = 0; i < it->second.size(); ++i)
      out << ' ' << it->second[i];
    out << endl;
  }
}

void chdl::get_input_nodes(std::set<nodeid_t> &s) {
  for (auto it = inputs.begin(); it != inputs.end(); ++it)
    for (unsigned i = 0; i < it->second.size(); ++i)
      s.insert(it->second[i]);
}

vector<node> chdl::input_internal(std::string name, unsigned n) {
  inputs[name] = vector<node>();

  for (unsigned i = 0; i < n; ++i) {
    inputimpl *n = new inputimpl(name, i);
    inputs[name].push_back(n->id);
  }

  return inputs[name];
}
