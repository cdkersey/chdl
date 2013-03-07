// This is the CHDL programmer facing side of the optimization layer. It is used
// solely for the purpose of invoking the optimization layer.
#include "vis.h"
#include "tap.h"
#include "gates.h"
#include "nodeimpl.h"
#include "gatesimpl.h"
#include "regimpl.h"
#include "litimpl.h"
#include "netlist.h"
#include "lit.h"
#include "node.h"
#include "memory.h"

#include <vector>
#include <set>
#include <map>

using namespace chdl;
using namespace std;

void chdl::print_dot(ostream &out) {
  set<nodeid_t> taps;
  get_tap_nodes(taps);

  out << "digraph G {" << endl;
  for (size_t i = 0; i < nodes.size(); ++i) {
    nodeimpl &n(*nodes[i]);
    invimpl  *inv (dynamic_cast< invimpl*>(nodes[i]));
    nandimpl *nand(dynamic_cast<nandimpl*>(nodes[i]));
    regimpl  *reg (dynamic_cast< regimpl*>(nodes[i]));
    litimpl  *lit (dynamic_cast< litimpl*>(nodes[i]));

    if (taps.find(n.id) != taps.end()) {
      out << "  x" << n.id << " [shape=diamond];" << endl
          << "  " << n.id << " -> x" << n.id << ';' << endl;
    }

    if      (inv)  out << "  " << n.id <<        " [shape=point];" << endl;
    else if (nand) out << "  " << n.id <<       " [shape=circle];" << endl;
    else if (reg)  out << "  " << n.id <<       " [shape=square];" << endl;
    else if (lit)  out << "  " << n.id << " [shape=doublecircle];" << endl;
    else           out << "  " << n.id <<      " [shape=hexagon];" << endl;

    for (unsigned j = 0; j < n.src.size(); ++j)
      out << "  " << unsigned(n.src[j]) << " -> " << n.id << ';' << endl;

    if (reg)
      out << "  " << unsigned(reg->d) << " -> " << n.id << ';' << endl;
      
  }
  out << "}" << endl;
}
