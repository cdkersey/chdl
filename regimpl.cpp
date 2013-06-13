#include <map>

#include "node.h"
#include "gates.h"
#include "reg.h"
#include "regimpl.h"
#include "nodeimpl.h"

#include "hierarchy.h"

using namespace chdl;
using namespace std;

regimpl::regimpl(node d): q(0), next_q(0), d(d) {}

regimpl::~regimpl() {}

bool regimpl::eval() { return q; }

void regimpl::print(ostream &out) {
  out << "  reg " << d << ' ' << id << endl;
}

void regimpl::print_vl(ostream &out) {
  out << "  reg __x" << id << ';' << endl
      << "  initial" << endl
      << "    begin" << endl
      << "      __x" << id << " <= 0;" << endl
      << "    end" << endl
      << "  always @ (posedge phi)" << endl
      << "    begin" << endl
      << "      __x" << id << " <= " << "__x" << d << ';' << endl
      << "    end" << endl << endl;
}

void regimpl::print_c_decl(ostream &out) {
  out << "  char r" << id << " = 0;\n";
}

void regimpl::print_c_impl(ostream &out) {
  out << "    regs_to[" << rid << "] = ";
  nodes[d]->print_c_val(out);
  out << ";\n";
}

void regimpl::print_c_val(ostream &out) {
  out << "regs_from[" << rid << ']';
}

void regimpl::assign_rids() {
  size_t count(0);
  for (nodeid_t i = 0; i < nodes.size(); ++i)
    if (regimpl* r = dynamic_cast<regimpl*>(nodes[i]))
      r->rid = count++;
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

node chdl::Wreg(node w, node d) {
  HIERARCHY_ENTER();
  node q;
  q = Reg(Mux(w, q, d));
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
