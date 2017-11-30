#include "node.h"
#include "nodeimpl.h"
#include "trisimpl.h"

#include <vector>
#include <iostream>
#include <cstdlib>

using namespace chdl;
using namespace std;

void tristateimpl::connect(node in, node enable) {
  src.push_back(in);
  src.push_back(enable);
  // assert(src.size() % 2 == 0);
}

bool tristateimpl::eval(cdomain_handle_t cd) {
  unsigned nDriven(0);
  bool rval(true);
  for (unsigned i = 0; i < src.size(); i += 2) {
    nodeimpl *pi(nodes[src[i]]), *pe(nodes[src[i+1]]);
    if (pe->eval(cd)) { ++nDriven; rval = pi->eval(cd); }
  }

  // Arbitrary if multiple drivers.
  if (nDriven > 1) rval = 0;
  return rval;
}

void tristateimpl::print(ostream &os) {
  os << "  tri";
  for (unsigned i = 0; i < src.size(); ++i) os << ' ' << src[i];
  os << ' ' << id << endl;
}

void tristateimpl::print_vl(ostream &os) {
  for (unsigned i = 0; i < src.size(); i += 2) {
    os << "  bufif1 __t" << id << '_' << i/2 << "(__x" << id << ", "
       << "__x" << src[i] << ", __x" << src[i+1] << ");" << endl;
  }
}

void tristateimpl::predecessors(print_lang l, print_phase p, set<printable*> &s)
{
  s.clear();
}

bool tristateimpl::is_initial(print_lang l, print_phase p) {
  return p == 100 || (l == PRINT_LANG_VERILOG && (p == 9 || p == 10));
}

void tristateimpl::print(ostream &out, print_lang l, print_phase p) {
  if (l == PRINT_LANG_VERILOG) {
    if (p == 9 || p == 10) {
      nodeimpl::print(out, l, p);
    } else if (p == 100) {
      for (unsigned i = 0; i < src.size()/2; ++i) {
        out << "  bufif1 __t" << id << '_' << i << "(__x" << id << ","
	    << "__x" << src[i*2] << ", __x" << src[i*2 + 1] << ");" << endl;
      }
    }
  } else if (l == PRINT_LANG_NETLIST && p == 100) {
    out << "  tri";
    for (auto &n : src) out << ' ' << n;
    out << ' ' << id << endl;
  }
}
