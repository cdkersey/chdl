#include <map>

#include "reg.h"
#include "regimpl.h"

using namespace chdl;
using namespace std;

map<nodeid_t, nodeid_t> regs;
typedef map<nodeid_t, nodeid_t>::iterator regs_it_t;

void reg::connect(node d) {
  static_cast<regimpl*>(nodes[idx])->connect(d);
  regs[idx] = d;
}

regimpl::regimpl(): q(0), next_q(0) {
  regs[id] = NO_NODE;
}

regimpl::~regimpl() {
  regs.erase(regs.find(id));
}

bool regimpl::eval() { return q; }

void regimpl::print(ostream &out) {
  out << "  reg " << d << ' ' << src[0] << endl;
}

reg chdl::Reg() { return (new regimpl())->id; }

void chdl::get_reg_nodes(set<nodeid_t> &s) {
  // Place all register Q and D nodes into the given set.
  for (regs_it_t i = regs.begin(); i != regs.end(); ++i) {
    s.insert(i->first);  // Q
    s.insert(i->second); // D
  }
}
