// This interface allows values to be assigned names so that they can be watched
// during simulation.
#include <map>
#include <set>
#include <vector>
#include <iostream>

#include "cdomain.h"
#include "tap.h"
#include "nodeimpl.h"
#include "printable.h"
#include "hierarchy.h"

#include "reset.h"

using namespace chdl;
using namespace std;

typedef map<string, vector<node> > taps_t;
typedef taps_t::iterator taps_it;

taps_t taps;
set<string> output_taps, io_taps;
vector<node> ghost_taps;

struct tap_t : public printable {
  tap_t() {}

  tap_t(string name) : name(name) {
    register_print_phase(PRINT_LANG_VERILOG, 9);
    register_print_phase(PRINT_LANG_VERILOG, 200);
  }

  bool is_initial(print_lang l, print_phase p) {
    if ((l == PRINT_LANG_VERILOG) && (p == 9 || p == 10 || p == 200))
      return true;

    if (p == 1100 && (output_taps.count(name) || l == PRINT_LANG_NETLIST) &&
	!io_taps.count(name))
      return true;

    if (p == 1200 && io_taps.count(name)) return true;
    
    return false;
  }

  void print(ostream &out, print_lang l, print_phase p) {
    if (l == PRINT_LANG_VERILOG) {
      if (p == 9) {
	if (io_taps.count(name)) {
          out << "  inout ";
	} else if (output_taps.count(name)) {
	  out << "  output ";
	}

	if (io_taps.count(name) || output_taps.count(name)) {
	  if (taps[name].size() > 1) {
	    out << '[' << taps[name].size() - 1 << ":0] ";
	  }
          out << name << ';' << endl;
        }
      } else if (p == 10) {
	out << "  wire ";
	if (taps[name].size() > 1) {
	  out << '[' << taps[name].size() - 1 << ":0] ";
	}
	out << name << ';' << endl;
      } else if (p == 200) {
	if (taps[name].size() == 1) {
	  out << "  assign " << name << " = __x" << taps[name][0] << ';'
	      << endl;
	} else if (taps[name].size() > 1) {
	  for (unsigned i = 0; i < taps[name].size(); ++i) {
	    out << "  assign " << name << '[' << i << "] = __x" << taps[name][i]
                << ';' << endl;
	  }
	}
      } else if (p == 1100 || p == 1200) {
	out << "  " << name;
      }
    } else if (l == PRINT_LANG_NETLIST && (p == 1100 || p == 1200)) {
      out << "  " << name;
      for (auto &n : taps[name]) out << ' ' << n;
      out << endl;
    }
  }

  void predecessors(print_lang l, print_phase p, set<printable *> &s) {
    s.clear();
  }

  string name;
};

map<string, tap_t> tap_printers;

static void reset_taps() {
  ghost_taps.clear();
  taps.clear();
  output_taps.clear();
  io_taps.clear();
}
CHDL_REGISTER_RESET(reset_taps);

void chdl::tap(std::string name, tristatenode t, bool output) {
  taps[name].push_back(t);
  if (!tap_printers.count(name)) tap_printers[name] = tap_t(name);
  if (output) io_taps.insert(name);
}

void chdl::tap(string name, node node, bool output) {
  taps[name].push_back(node);
  if (!tap_printers.count(name)) tap_printers[name] = tap_t(name);
  if (output) output_taps.insert(name);
}

void chdl::gtap(node n) {
  ghost_taps.push_back(n);
}

void chdl::print_taps_vl_head(std::ostream &out) {
  for (auto t : taps)
    if (output_taps.count(t.first) || io_taps.count(t.first))
      out << ',' << endl << "  " << t.first;
}

void chdl::print_taps_vl_body(std::ostream &out, bool print_non_output) {
  for (auto t : taps) {
    if (output_taps.count(t.first))
      out << "  output ";
    else if (io_taps.count(t.first))
      out << "  inout ";
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
    // TODO: io taps, output taps, and just plain taps need to get more clearly
    //       distinguished in the netlist format. I hate the following line
    if (io_taps.count(t.first)) continue;

    out << "  " << t.first;
    for (size_t i = 0; i < t.second.size(); ++i)
      out << ' ' << t.second[i];
    out << endl;
  }
}

// TODO: see comment in print_tap_nodes
void chdl::print_io_tap_nodes(ostream &out) {
  for (auto t : taps) {
    if (io_taps.count(t.first)) {
      out << "  " << t.first;
      for (size_t i = 0; i < t.second.size(); ++i)
        out << ' ' << t.second[i];
      out << endl;
    }
  }
}

void chdl::print_taps(ostream &out, cdomain_handle_t cd) {
  for (auto t : taps) {
    if (t.second.size() > 1) out << 'b';
    for (int j = t.second.size()-1; j >= 0; --j)
      out << (nodes[t.second[j]]->eval(cd) ? '1' : '0');
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
  for (auto t : ghost_taps)
    s.insert(t);
}

void chdl::get_tap_map(std::map<nodeid_t, std::string> &m) {
  m.clear();
  for (auto t : taps)
    for (auto n : t.second)
      m[n] = t.first;
}
