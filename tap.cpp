// This interface allows values to be assigned names so that they can be watched
// during simulation.
#include <map>
#include <vector>
#include <iostream>

#include "tap.h"
#include "nodeimpl.h"

#include "hierarchy.h"

using namespace chdl;
using namespace std;

typedef map<string, vector<node> > taps_t;
typedef taps_t::iterator taps_it;

taps_t taps;

void chdl::tap(string name, node node) {
  taps[name].push_back(node);
}

void chdl::print_taps_vl_head(std::ostream &out) {
  for (auto it = taps.begin(); it != taps.end(); ++it)
    out << ',' << endl << "  " << it->first;
}

void chdl::print_taps_vl_body(std::ostream &out) {
  for (auto it = taps.begin(); it != taps.end(); ++it) {
    out << "  output ";
    if (it->second.size() > 1)
      out << '[' << it->second.size()-1 << ":0] ";
    out << it->first << ';' << endl;
    if (it->second.size() == 1) {
      out << "  assign " << it->first << " = __x" << it->second[0] << ';'
          << endl;
    } else {
      for (unsigned i = 0; i < it->second.size(); ++i) {
        out << "  assign " << it->first << '[' << i << "] = __x"
            << it->second[i] << ';' << endl;
      }
    }
  }
}

void chdl::print_tap_nodes(ostream &out) {
  for (auto it = taps.begin(); it != taps.end(); ++it) {
    out << "  " << it->first;
    for (size_t i = 0; i < it->second.size(); ++i)
      out << ' ' << it->second[i];
    out << endl;
  }
}

void chdl::print_taps(ostream &out) {
  for (taps_it i = taps.begin(); i != taps.end(); ++i) {
    if (i->second.size() > 1) out << 'b';
    for (int j = i->second.size()-1; j >= 0; --j)
      out << nodes[i->second[j]]->eval();
    if (i->second.size() > 1) out << ' ';
    out << i->first << endl;
  }
}

void chdl::print_vcd_header(ostream &out) {
  out << "$timescale 1 ns $end" << endl;

  for (taps_it i = taps.begin(); i != taps.end(); ++i)
    out << "$var reg " << i->second.size() << ' ' << i->first << ' '
         << i->first << " $end" << endl;

  out << "$enddefinitions $end" << endl;
}

void chdl::get_tap_nodes(set<nodeid_t> &s) {
  for (taps_it i = taps.begin(); i != taps.end(); ++i)
    for (unsigned j = 0; j < i->second.size(); ++j)
      s.insert(i->second[j]);
}
