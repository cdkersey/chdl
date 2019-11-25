#include <cstdlib>
#include <iostream>
#include <string>
#include <map>
#include <vector>

#include "printable.h"
#include "input.h"
#include "nodeimpl.h"
#include "reset.h"

using namespace std;
using namespace chdl;

map <string, vector<node> > inputs;

struct input_printer : public printable {
  input_printer() {}
  
  input_printer(string name): name(name) {
    register_print_phase(PRINT_LANG_VERILOG, 9);
    register_print_phase(PRINT_LANG_VERILOG, 10);
  }
  
  bool is_initial(print_lang l, print_phase p) {
    return ((l == PRINT_LANG_VERILOG) && ((p == 9) || (p == 10)))
      || (p == 100) || (p == 1000);
  }

  void predecessors(print_lang, print_phase, set<printable *> &s) {
    s.clear();
  }
  
  void print(ostream &out, print_lang l, print_phase p) {
    unsigned size = (inputs[name].size());

    if (l == PRINT_LANG_VERILOG) {
      if (p == 9 || p == 10) {
        if (p == 9) out << "  input ";
	else        out << "  wire ";
	if (size > 1) out << '[' << size-1 << ":0] ";
	out << ' ' << name << ';' << endl;
      } else if (p == 100) {
        if (size == 1) {
	  out << "  assign __x" << inputs[name][0] << " = " << name << ';'
	       << endl;
	} else {
	  for (unsigned i = 0; i < size; ++i) {
	    out << "  assign __x" << inputs[name][i] << " = " << name << '['
	        << i << "];" << endl;
	  }
	}
      } else if (p == 1000) {
        out << "  " << name;
      }
    } else if (l == PRINT_LANG_NETLIST) {
      if (p == 1000) {
        out << "  " << name;
        for (auto &n : inputs[name]) out << ' ' << n;
      }
    }
  }
  
  string name;
};

map <string, input_printer > input_printers; 

static void clear_inputs() { inputs.clear(); }
CHDL_REGISTER_RESET(clear_inputs);

class inputimpl : public nodeimpl {
  public:
    inputimpl(string n, int i=-1): nodeimpl(), name(n), pos(i) {}
    
    bool eval(cdomain_handle_t) {
      return 0;
    }

    bool is_initial(print_lang l, print_phase p) {
      return (p == 100);
    }
  
    void print(ostream &) {}
    void print_vl(ostream &);

  private:
    string name;         // Name of input in input map
    int pos;             // Position in vector in input map. -1 for no name
};

void inputimpl::print_vl(ostream &out) {
  out << "  assign __x" << id << " = " << name;
  if (inputs[name].size() > 1)
    out << '[' << pos << ']';
  out << ';' << endl;
}

node chdl::Input(std::string name) {
  inputimpl *n = new inputimpl(name, -1);
  inputs[name] = vector<node>(1, n->id);
  input_printers[name] = input_printer(name);
  return node(n->id);
}

void chdl::print_inputs_vl_head(std::ostream &out) {
  for (auto i : inputs)
    out << ',' << endl << "  " << i.first;
}

void chdl::print_inputs_vl_body(std::ostream &out) {
  for (auto in : inputs) {
    out << "  input ";
    if (in.second.size() > 1)
      out << '[' << in.second.size()-1 << ":0] ";
    out << in.first << ';' << endl;

    #if 0
    if (in.second.size() > 1) {
      for (unsigned i = 0; i < in.second.size(); ++i) {
        out << "  assign __x" << in.second[i] << " = "
            << in.first << '[' << i << "];" << endl;
      }
    } else {
      out << "  assign __x" << in.second[0] << " = "
          << in.first << ';' << endl;
    }
    #endif
  }
}

void chdl::print_input_nodes(std::ostream &out) {
  for (auto in : inputs) {
    out << "  " << in.first;
    for (unsigned i = 0; i < in.second.size(); ++i)
      out << ' ' << in.second[i];
    out << endl;
  }
}

void chdl::get_input_nodes(std::set<nodeid_t> &s) {
  for (auto in : inputs)
    for (unsigned i = 0; i < in.second.size(); ++i)
      s.insert(in.second[i]);
}

vector<node> chdl::input_internal(std::string name, unsigned n) {
  inputs[name] = vector<node>();
  input_printers[name] = input_printer(name);

  for (unsigned i = 0; i < n; ++i) {
    inputimpl *n = new inputimpl(name, i);
    inputs[name].push_back(n->id);
  }

  return inputs[name];
}
