// Node implementation.
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <iostream>
#include <functional>
#include <string>
#include <typeinfo>
#include <sstream>

#include "reset.h"
#include "printable.h"

using namespace chdl;
using namespace std;

set<printable*> &chdl::printables() {
  static std::set<printable*> *r = new std::set<printable*>();
  return *r;
}

map<print_lang, set<print_phase> > &phases() {
  static map<print_lang, set<print_phase> > phases;
  return phases;
}

static void clear_printables() {
  printables().clear();
  phases().clear();
}
CHDL_REGISTER_RESET(clear_printables);

void chdl::printable::register_print_phase(print_lang l, print_phase p) {
  phases()[l].insert(p);
}

void chdl::get_initial(print_lang l, print_phase p, set<printable*> &s) {
  s.clear();
  for (auto &printable : printables())
    if (printable->is_initial(l, p))
      s.insert(printable);
}

void print_all(const function<void(string)> &f, print_lang l, print_phase phase)
{
  set<printable *> initial;
  get_initial(l, phase, initial);
    
  map<printable*, set<printable *> > successors;
  for (auto &p : printables()) {
    set<printable*> pred;
    p->predecessors(l, phase, pred);
    for (auto &q : pred)
      successors[q].insert(p);
  }

  map<printable*, int> ll;
  for (auto &p : initial) ll[p] = 0;

  // Find evaluation order.
  bool changed;
  do {
    changed = false;
      
    for (auto &x : ll) {
      for (auto &s : successors[x.first]) {
        if (ll[s] <= x.second) {
          ll[s] = x.second + 1;
          changed = true;
        }
      }
    }
  } while (changed);

  map<int, set<printable *> > order;
  for (auto &x : ll) order[x.second].insert(x.first);

  for (auto &x : order) {
    for (auto &p : x.second) {
      ostringstream oss;
      p->print(oss, l, phase);
      f(oss.str());
    }
  }
}

void print_all(ostream &out, print_lang l, print_phase phase) {
  print_all([&out](string s){ out << s; }, l, phase);
}

struct top_level_printable : public printable {
  top_level_printable(print_lang l): l(l) {}

  virtual bool is_initial(print_lang l, print_phase p) {
    return (p == 0) || (p == 1);
  }

  virtual void predecessors(print_lang, print_phase, std::set<printable *> &s)
  {
    s.clear();
  }
   
  virtual void print(std::ostream &out, print_lang l, print_phase p) {
    if (l == PRINT_LANG_NETLIST) {
      if (p == 0) {
	vector<string> inputs;
        out << "inputs" << endl;
        print_all([&out](string s){ out << s << endl; }, l, 1000);

        out << "outputs" << endl;
        print_all(out, l, 1100);

        out << "inout" << endl;
        print_all(out, l, 1200);

        out << "design" << endl;
      }
    } else if (l == PRINT_LANG_VERILOG) {
      if (p == 0) {
	vector<string> args;
	auto append = [&args](string s){ args.push_back(s); };
        print_all(append, l, 1000);
        print_all(append, l, 1100);
        print_all(append, l, 1200);

	out << "module chdl_design(reset";
	for (unsigned i = 0; i < args.size(); ++i) {
          out << ',' << args[i];
	}
	out << endl << ");" << endl << "  input reset;" << endl << "  wire reset;" << endl;
      } else if (p == 1) {
	out << "endmodule" << endl;
      }
    }
  }

  print_lang l;
};

void chdl::print_design(std::ostream &out, print_lang l) {
  top_level_printable top(l);
  
  print_all(out, l, 0);

  for (auto &phase : phases()[l]) {
    print_all(out, l, phase);
  }

  print_all(out, l, 1);
}


