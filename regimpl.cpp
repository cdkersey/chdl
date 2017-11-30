#include <map>

#include "node.h"
#include "gates.h"
#include "reg.h"
#include "regimpl.h"
#include "nodeimpl.h"

#include "hierarchy.h"

using namespace chdl;
using namespace std;

regimpl::regimpl(node d): q(0), next_q(0), d(d), cd(cur_clock_domain()) {}

regimpl::~regimpl() {}

bool regimpl::eval(cdomain_handle_t) { return q; }

void regimpl::print(ostream &out) {
  if (cd == 0) {
    out << "  reg " << d << ' ' << id << endl;
  } else {
    out << "  reg<" << cd << "> " << d << ' ' << id << endl;
  }
}

bool regimpl::is_initial(print_lang l, print_phase p) {
  if (p >= 1000000 || p == 100 || p == 10) return true;

  return false;
}

void regimpl::print_vl(ostream &out) {
  const bool reset_signal(true), level_trig_reset(true);

  if (!reset_signal) {
    out << "  initial" << endl
        << "    begin" << endl
        << "      __x" << id << " <= 0;" << endl
        << "    end" << endl;
  }

  if (cd == 0)
    out << "  always @ (posedge phi";
  else
    out << "  always @ (posedge phi" << cd;

  if (reset_signal) {
    out << " or ";
    if (!level_trig_reset) out << "posedge ";
    out << "reset";
  }
  
  out << ')' << endl << "    begin" << endl;
  
  if (reset_signal)
    out << "    if (reset) __x" << id << " <= 0;" << endl
        << "    else if (phi) ";
  else
    out << "    ";
  
  out << "__x" << id << " <= " << "__x" << d << ';' << endl
      << "    end" << endl;
}

void regimpl::print(std::ostream &out, print_lang l, print_phase p) {
  if (l == PRINT_LANG_NETLIST) {
    if (p == 100) {
      if (cd == 0)
        out << "  reg " << d << ' ' << id << endl;
      else
	out << "  reg<" << cd << "> " << d << ' ' << id << endl;
    }
  } else if (l == PRINT_LANG_VERILOG) {
    if (p == 10) {
      out << "  reg __x" << id << ';' << endl;
    } else if (p == 1000000 + cd) { // Rising edge for clock domain
      out << "      __x" << id << " <= __x" << d << ';' << endl;
    } else if (p == 2000000 + cd) { // Global reset signal
      out << "          __x" << id << " <= 0;" << endl;
    }
  }
}

node chdl::Reg(node d, bool val) {
  HIERARCHY_ENTER();  
  if (val) {
    node r((new regimpl(Inv(d)))->id);
    HIERARCHY_EXIT();
    return Inv(r);
  } else {
    node r((new regimpl(d))->id);
    HIERARCHY_EXIT();
    return r;
  }
}

node chdl::Wreg(node w, node d, bool initial) {
  HIERARCHY_ENTER();
  node q;
  q = Reg(Mux(w, q, d), initial);
  HIERARCHY_EXIT();
  return q;
}

void chdl::get_reg_nodes(set<nodeid_t> &s) {
  // We used to use a map<node, node> to keep track of all registers. This
  // requires less maintenance:
  for (size_t i = 0; i < nodes.size(); ++i) {
    regimpl *r(dynamic_cast<regimpl*>(nodes[i]));
    if (r) { s.insert(r->id); s.insert(r->d); }
  }
}

void chdl::get_reg_q_nodes(set<nodeid_t> &s) {
  for (size_t i = 0; i < nodes.size(); ++i) {
    regimpl *r(dynamic_cast<regimpl*>(nodes[i]));
    if (r) s.insert(r->id);
  }
}

void chdl::get_reg_d_nodes(set<nodeid_t> &s) {
  for (size_t i = 0; i < nodes.size(); ++i) {
    regimpl *r(dynamic_cast<regimpl*>(nodes[i]));
    if (r) s.insert(r->d);
  }
}
