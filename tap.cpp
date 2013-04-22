// This interface allows values to be assigned names so that they can be watched
// during simulation.
#include <map>
#include <set>
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
set<string> output_taps;

void chdl::tap(string name, node node, bool output) {
  taps[name].push_back(node);
  if (output) output_taps.insert(name);
}

void chdl::print_taps_vl_head(std::ostream &out) {
  for (auto t : taps)
    if (output_taps.find(t.first) != output_taps.end())
      out << ',' << endl << "  " << t.first;
}

void chdl::print_taps_vl_body(std::ostream &out, bool print_non_output) {
  for (auto t : taps) {
    if (output_taps.find(t.first) != output_taps.end())
      out << "  output ";
    else if (print_non_output)
      out << "  wire ";
    else
      continue;

    if (t.second.size() > 1)
      out << '[' << t.second.size()-1 << ":0] ";
    out << t.first << ';' << endl;

    if (t.second.size() == 1) {
      out << "  assign " << t.first << " = __x" << t.second[0] << ';'
          << endl;
    } else {
      for (unsigned i = 0; i < t.second.size(); ++i) {
        out << "  assign " << t.first << '[' << i << "] = __x"
            << t.second[i] << ';' << endl;
      }
    }
  }
}

void chdl::print_tap_nodes(ostream &out) {
  for (auto t : taps) {
    out << "  " << t.first;
    for (size_t i = 0; i < t.second.size(); ++i)
      out << ' ' << t.second[i];
    out << endl;
  }
}

void chdl::print_taps(ostream &out) {
  for (auto t : taps) {
    if (t.second.size() > 1) out << 'b';
    for (int j = t.second.size()-1; j >= 0; --j)
      out << nodes[t.second[j]]->eval();
    if (t.second.size() > 1) out << ' ';
    out << t.first << endl;
  }
}

void chdl::print_vcd_header(ostream &out) {
  out << "$timescale 1 ns $end" << endl;

  for (auto t : taps)
    out << "$var reg " << t.second.size() << ' ' << t.first << ' '
         << t.first << " $end" << endl;

  out << "$enddefinitions $end" << endl;
}

void chdl::get_tap_nodes(set<nodeid_t> &s) {
  for (auto t : taps)
    for (unsigned j = 0; j < t.second.size(); ++j)
      s.insert(t.second[j]);
}
