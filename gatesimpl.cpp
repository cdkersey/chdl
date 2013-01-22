#include "gatesimpl.h"

using namespace std;
using namespace chdl;

bool invimpl::eval() {
  return !(nodes[src[0]]->eval());
}

void invimpl::print(ostream &out) {
  out << "  inv " << src[0] << ' ' << id << endl;
}

bool nandimpl::eval() {
  return !(nodes[src[0]]->eval() && nodes[src[1]]->eval());
}

void nandimpl::print(ostream &out) {
  out << "  nand " << src[0] << ' ' << src[1] << ' ' << id << endl;
}
