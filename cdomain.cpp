#include "cdomain.h"
#include "tickable.h"
#include "sim.h"
#include "printable.h"
#include "nodeimpl.h"

#include <set>
#include <vector>
#include <stack>
#include <iostream>
#include <map>

using namespace chdl;
using namespace std;

struct cdomain_t;

struct cdomain_t : public printable {
  cdomain_t(cdomain_handle_t id): id(id) {
    register_print_phase(PRINT_LANG_VERILOG, 100);
    register_print_phase(PRINT_LANG_VERILOG, 9);
    register_print_phase(PRINT_LANG_VERILOG, 10);
  }

  virtual bool is_initial(print_lang l, print_phase p) {
    if (l == PRINT_LANG_VERILOG) {
      if (p == 9) return true;
      if (p == 10) return true;
      if (p == 100) return true;
      if (p == 1000) return true;
    }

    return false;
  }

  virtual void predecessors(print_lang l, print_phase p, set<printable*> &s) {
    s.clear();
  }

  virtual void print(ostream &out, print_lang l, print_phase p) {
    if (l == PRINT_LANG_VERILOG && p == 9) {
      out << "  input phi";
      if (id > 0) out << id;
      out << ';' << endl;
    } else if (l == PRINT_LANG_VERILOG && p == 10) {
      out << "  wire phi";
      if (id > 0) out << id;
      out << ';' << endl;
    } else if (l == PRINT_LANG_VERILOG && p == 100) {
      out << "  // clock domain " << id << endl
          << "  always @ (posedge phi";
      if (id > 0) out << id;
      out << ')' << endl << "    begin" << endl;
      for (auto &t : tickables()[id]) {
	printable *p;
	if (p = dynamic_cast<printable*>(t)) {
	  p->print(out, l, 1000000 + id);
	}
      }

      out << "      if (reset)" << endl << "        begin" << endl;
      for (auto &t : tickables()[id]) {
	printable *p;
	if (p = dynamic_cast<printable*>(t)) {
	  p->print(out, l, 2000000 + id);
	}
      }
      out << "        end" << endl;
      out << "    end" << endl << endl;
    } else if (l == PRINT_LANG_VERILOG && p == 1000) {
      out << "  phi";
      if (id > 0) out << id;
    }
  }

  cdomain_handle_t id;
};

cdomain_t cd0(0);

cdomain_handle_t &chdl::cur_clock_domain() {
  static cdomain_handle_t cd(0);
  return cd;
}

vector<unsigned> &chdl::tick_intervals() {
  static vector<unsigned> ti{1};
  return ti;
}

cdomain_handle_t chdl::new_clock_domain(unsigned interval) {
  cdomain_handle_t h(tick_intervals().size());
  tick_intervals().push_back(interval);
  tickables().push_back(vector<tickable*>());
  new cdomain_t(h);
  now.push_back(0);
  return h;  
}

void chdl::set_clock_domain(cdomain_handle_t cd) {
  cur_clock_domain() = cd;
}

static vector<cdomain_handle_t> &cdomainstack() {
  static vector<cdomain_handle_t> s{0};
  return s;
}

cdomain_handle_t chdl::push_clock_domain(unsigned interval) {
  set_clock_domain(new_clock_domain(interval));
  cdomainstack().push_back(cur_clock_domain());
  return cur_clock_domain();
}

cdomain_handle_t chdl::pop_clock_domain() {
  cdomainstack().pop_back();
  cur_clock_domain() = *(cdomainstack().rbegin());
  return cur_clock_domain();
}

unsigned chdl::clock_domains() { return tickables().size(); }
