#include "gatesimpl.h"

using namespace std;
using namespace chdl;

bool invimpl::eval() {
  if (t_cval != sim_time()) {
    cval = !(nodes[src[0]]->eval());
    t_cval = sim_time();
  }

  return cval;
}

void invimpl::print(ostream &out) {
  out << "  inv " << src[0] << ' ' << id << endl;
}

bool nandimpl::eval() {
  if (t_cval != sim_time()) {
    cval =  !(nodes[src[0]]->eval() && nodes[src[1]]->eval());
    t_cval = sim_time();
  }

  return cval;
}

void nandimpl::print(ostream &out) {
  out << "  nand " << src[0] << ' ' << src[1] << ' ' << id << endl;
}
