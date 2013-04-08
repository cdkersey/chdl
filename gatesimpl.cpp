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

void invimpl::print_vl(ostream &out) {
  out << "  wire __x" << id << ';' << endl
      << "  not __i" << id << "(__x" << id << ", " << "__x" << src[0] << ");"
      << endl;
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

void nandimpl::print_vl(ostream &out) {
  out << "  wire __x" << id << ';' << endl
      << "  nand __n" << id << "(__x" << id << ", __x" << src[0] << ", __x"
      << src[1] << ");" << endl;
}
