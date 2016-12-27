#include "gatesimpl.h"

using namespace std;
using namespace chdl;

bool invimpl::eval(cdomain_handle_t cd) {
  if (t_cval != sim_time(cd)) {
    cval = !(nodes[src[0]]->eval(cd));
    t_cval = sim_time(cd);
  }

  return cval;
}

void invimpl::print(ostream &out) {
  out << "  inv " << src[0] << ' ' << id << endl;
}

void invimpl::print_vl(ostream &out) {
  out << "  not __i" << id << "(__x" << id << ", " << "__x" << src[0] << ");"
      << endl;
}

void invimpl::print(std::ostream &out, print_lang l, print_phase p) {
  nodeimpl::print(out, l, p);

  if (p == 100) {
    if (l == PRINT_LANG_NETLIST) {
      out << "  inv " << src[0] << ' ' << id << endl;
    } else if (l == PRINT_LANG_VERILOG) {
      out << "  not __i" << id << "(__x" << id << ", __x" << src[0] << ");"
	  << endl;
    }
  }
}

bool nandimpl::eval(cdomain_handle_t cd) {
  if (t_cval != sim_time(cd)) {
    cval =  !(nodes[src[0]]->eval(cd) && nodes[src[1]]->eval(cd));
    t_cval = sim_time(cd);
  }

  return cval;
}

void nandimpl::print(ostream &out) {
  out << "  nand " << src[0] << ' ' << src[1] << ' ' << id << endl;
}

void nandimpl::print(std::ostream &out, print_lang l, print_phase p) {
  nodeimpl::print(out, l, p);

  if (p == 100) {
    if (l == PRINT_LANG_NETLIST) {
      out << "  nand " << src[0] << ' ' << src[1] << ' ' << id << endl;
    } else if (l == PRINT_LANG_VERILOG) {
      out << "  nand __n" << id << "(__x" << id << ", __x" << src[0] << ", __x"
          << src[1] << ");" << endl;
    }
  }
}

void nandimpl::print_vl(ostream &out) {
  out << "  nand __n" << id << "(__x" << id << ", __x" << src[0] << ", __x"
      << src[1] << ");" << endl;
}
