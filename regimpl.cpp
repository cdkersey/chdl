#include <map>

#include "reg.h"
#include "regimpl.h"
#include "nodeimpl.h"

using namespace chdl;
using namespace std;

void reg::connect(node d) {
  static_cast<regimpl*>(nodes[idx])->connect(d);
}

regimpl::regimpl(): q(0), next_q(0) {}

regimpl::~regimpl() {}

bool regimpl::eval() { return q; }

void regimpl::print(ostream &out) {
  out << "  reg " << d << ' ' << id << endl;
}

reg chdl::Reg() { return (new regimpl())->id; }

void chdl::get_reg_nodes(set<nodeid_t> &s) {
  // We used to use a map<node, node> to keep track of all registers. This
  // requires less maintenance:
  for (size_t i = 0; i < nodes.size(); ++i) {
    regimpl *r(dynamic_cast<regimpl*>(nodes[i]));
    if (r) { s.insert(r->id); s.insert(r->d); }
  }
}
