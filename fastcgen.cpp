#include "analysis.h"
#include "nodeimpl.h"
#include "gatesimpl.h"
#include "regimpl.h"
#include "reg.h"
#include "hierarchy.h"
#include "netlist.h"
#include "tap.h"

using namespace std;
using namespace chdl;

void print_c_boilerplate_top(ostream &out);
void print_c_boilerplate_bottom(ostream &out);

void chdl::print_c(ostream &out) {
  const unsigned BITS(32);

  // // // Generate internal representation // // //

  // First, compute "logic layer" of each node, its distance from the farthest
  // register or literal. This is used to determine the order in which node
  // values are computed.
  map<nodeid_t, int> ll;
  multimap<int, nodeid_t> ll_r;
  set<nodeid_t> regs;
  get_reg_nodes(regs);
  int max_ll(0);
  for (auto r : regs) ll[r] = 0;
  bool changed;
  do {
    changed = false;

    for (nodeid_t i = 0; i < nodes.size(); ++i) {
      int new_ll(0);
      for (unsigned j = 0; j < nodes[i]->src.size(); ++j) {
        nodeid_t s(nodes[i]->src[j]);
        if (ll.find(s) != ll.end() && ll[s] >= new_ll)
          new_ll = ll[s] + 1;
      }
      if (ll.find(i) == ll.end() || new_ll > ll[i]) {
        changed = true; 
        ll[i] = new_ll;
      }
      if (new_ll > max_ll) max_ll = new_ll;
    }
  } while(changed);

  for (auto l : ll) ll_r.insert(pair<int, nodeid_t>(l.second, l.first));

  // The internal representation for the logic. "WIRE" means that a signal
  // passes through a given logic level.
  enum nodetype_t { NAND, INV, WIRE, DUMMY };
  struct cnode_t {
    cnode_t() : type(DUMMY), node(0), width(1) {}
    nodetype_t type;
    nodeid_t node;
    unsigned width;
    vector<unsigned> src;
  };

  // Generate the internal representation of the gates
  vector<vector<cnode_t>> cnodes(max_ll+1);
  for (auto x : ll_r) {
    int l(x.first), inputs(0);
    cnodes[l].push_back(cnode_t());
    cnode_t &cn(*cnodes[l].rbegin());

    cn.node = x.second;
    inputs = nodes[x.second]->src.size();
    
    if (nandimpl *p = dynamic_cast<nandimpl*>(nodes[x.second])) {
      cn.type = NAND;
    } else if (invimpl *p = dynamic_cast<invimpl*>(nodes[x.second])) {
      cn.type = INV;
    } else if (regimpl *r = dynamic_cast<regimpl*>(nodes[x.second])) {
      cn.type = WIRE;
    }

    for (unsigned i = 0; i < inputs; ++i) {
      nodeid_t n(nodes[x.second]->src[i]);
      if (ll.find(n) == ll.end()) {
        cerr << "Node " << n << " has no logic level.\n";
      }
      int i_ll(ll[n]);
      for (int j = l - 1; j > i_ll; --j) {
        bool already_present(false);
        for (unsigned k = 0; k < cnodes[j].size(); ++k)
          if (cnodes[j][k].node == n) already_present = true;
        if (already_present) continue;

        cnodes[j].push_back(cnode_t());
        cnode_t &icn(*cnodes[j].rbegin());
        icn.type = WIRE;
        icn.node = n;
      }
    }
  }

  // Add wires to connect register inputs
  for (auto r : cnodes[0]) {
    regimpl *ri = dynamic_cast<regimpl*>(nodes[r.node]);
    nodeid_t d(ri->d);
    if (ll.find(d) == ll.end())
      cerr << "Node " << d << " has no logic level.\n";
    int d_ll(ll[d]);
    for (int j = max_ll; j > d_ll; --j) {
      bool already_present(false);
      for (unsigned k = 0; k < cnodes[j].size(); ++k)
        if (cnodes[j][k].node == d) already_present = true;
      if (already_present) continue;

      cnodes[j].push_back(cnode_t());
      cnode_t &dcn(*cnodes[j].rbegin());
      dcn.type = WIRE;
      dcn.node = d;
    }
  }

  // Create a way to quickly look up node indices
  vector<map<nodeid_t, unsigned>> ll_idx(cnodes.size());
  for (unsigned i = 0; i < cnodes.size(); ++i)
    for (unsigned j = 0; j < cnodes[i].size(); ++j)
      ll_idx[i][cnodes[i][j].node] = j;

  // Set up src vectors
  for (unsigned i = 1; i < cnodes.size(); ++i) {
    for (unsigned j = 0; j < cnodes[i].size(); ++j) {
      if (cnodes[i][j].type == WIRE) {
        nodeid_t n(cnodes[i][j].node);
        cnodes[i][j].src.push_back(ll_idx[i-1][n]);
      } else {
        nodeid_t n(cnodes[i][j].node);
        for (unsigned k = 0; k < nodes[n]->src.size(); ++k) {
          cnodes[i][j].src.push_back(ll_idx[i-1][nodes[n]->src[k]]);
        }
      }
    }
  }

  // // // Optimize // // //

  // Set width field appropriately.
  for (unsigned i = 0; i < cnodes.size(); ++i) {
    unsigned input_lvl(i == 0 ? cnodes.size() - 1 : i-1);
    for (unsigned j = 0; j < cnodes[i].size() - 1; j += cnodes[i][j].width) {
      nodetype_t t(cnodes[i][j].type);
      for (unsigned k = j+1; k < cnodes[i].size(); ++k, ++cnodes[i][j].width) {
        if (cnodes[i][j].width == BITS) break;
        if (cnodes[i][k].type != t) break;

        if (t == DUMMY || i == 0) continue;

        if (t == NAND || t == INV || t == WIRE) {
          unsigned i00(cnodes[i][k-1].src[0]),
                   i01(t == NAND?cnodes[i][k-1].src[1]:0),
                   i10(cnodes[i][k].src[0]),
                   i11(t == NAND?cnodes[i][k].src[1]:0);
        
          if (i00 + 1 != i10) break;
          if (t == NAND && i01 + 1 != i11) break;
        }
      }
    }
  }

  // Erase redundant operations
  vector<vector<cnode_t>> cnodes2(max_ll+1);
  for (unsigned i = 0; i < cnodes.size(); ++i)
    for (unsigned j = 0; j < cnodes[i].size(); j += cnodes[i][j].width)
      cnodes2[i].push_back(cnodes[i][j]);
  cnodes = cnodes2;

  // // // Emit C code // // //

  print_c_boilerplate_top(out);
  print_taps_c_head(out);

  // Declare variables.
  out << "  uint" << BITS << "_t ";
  unsigned size0;
  for (unsigned l = 0; l < cnodes.size(); ++l) {
    unsigned total_bits(0);
    for (unsigned i = 0; i < cnodes[l].size(); ++i)
      total_bits += cnodes[l][i].width;
    unsigned sz((total_bits + BITS - 1)/BITS);
    if (l == 0) size0 = sz;
    out << "ll" << l << '[' << sz << ']';
    if (l != cnodes.size() - 1) out << ",\n           ";
  }
  out << ";\n"
      << "  unsigned long cycle;\n\n"
      << "  unsigned long start_time = time_us();\n\n"
      << "  memset(ll0, 0, sizeof(uint" << BITS << "_t)*" << size0 << ");\n";

  // Loop top
  out << "  for (cycle = 0; cycle < 1000; ++cycle) {\n";

  // Perform Boolean evaluation
  for (unsigned l = 1; l < cnodes.size(); ++l) {
    unsigned cur_idx(0);
    for (unsigned i = 0; i < cnodes[l].size(); ++i) {
      out << "    { ";

      // Gather inputs
      nodeid_t n(cnodes[l][i].node);
      unsigned w(cnodes[l][i].width);
      for (unsigned k = 0; k < cnodes[l][i].src.size(); ++k) {
        unsigned idx(cnodes[l][i].src[k]);
        if (k == 0) out << "uint" << BITS << "_t ";
        out << 'i' << k << " = ";
        for (unsigned i = idx; i < idx + w;) {
          unsigned chunk(BITS - i%BITS), remaining_bits(w - (i - idx));
          if (chunk > remaining_bits) chunk = remaining_bits;

          if (i != idx) out << " | ";

          out << "(((ll" << l-1 << '[' << i/BITS << ']';
          if (i%BITS) out << ">>" << (i%BITS);
          out << ')';

          if (chunk < BITS) out << '&' << ((1<<chunk)-1);
          out << ')';
          if (i - idx > 0) out << "<<" << i - idx;
          out << ')';
          i += chunk;
        }
        out << ", ";
      }

      // Perform computation
      out << "v = ";
      if (cnodes[l][i].type == WIRE) {
        out << "i0";
      } else if (cnodes[l][i].type == INV) {
        out << "~i0";
        if (w < BITS) out << '&' << ((1<<w)-1);
      } else if (cnodes[l][i].type == NAND) {
        out << "~(i0&i1)";
        if (w < BITS) out << '&' << ((1<<w)-1);
      }
      out << "; ";

      // Write value into output vector
      for (unsigned i = cur_idx; i < cur_idx + w;) {
        unsigned chunk(BITS - i%BITS), remaining_bits(w - (i - cur_idx));
        if (chunk > remaining_bits) chunk = remaining_bits;
        if (chunk == BITS) {
          out << "ll" << l << '[' << i/BITS << "] = v; ";
        } else {
          out << "ll" << l << '[' << i/BITS << "] &= ~"
              << (((1<<chunk)-1)<<(i%BITS)) << "; "
              << "ll" << l << '[' << i/BITS << "] |= (v";
          if (i - cur_idx > 0) out << ">>" << i - cur_idx;
          out << ')';
          if (i%BITS) out << " << " << (i%BITS);
          out << "; ";
        }
        i += chunk;
      }

      out << " }\n";

      cur_idx += w;
    }
  }

  // Print taps
  out << "    printf(\"#%lu\\n\", cycle);\n";
  map<string, vector<node>> &taps(get_taps());
  for (auto t : taps) {
    out << "    printf(\"b";
    for (auto n : t.second)
      out << "%d";
    out << ' ' << t.first;
    out << "\\n\"";
    for (int tap_idx = t.second.size()-1; tap_idx >= 0; --tap_idx) {
      nodeid_t n(t.second[tap_idx]);
      out << ", (";
      bool found(false);
      for (unsigned l = 0; l < cnodes.size() && !found; ++l) {
        if (ll_idx[l].find(n) == ll_idx[l].end()) continue;

        unsigned idx(0), found_idx(ll_idx[l][n]);
        for (unsigned i = 0; i < cnodes[l].size() && !found; ++i) {
          if (idx <= found_idx && idx + cnodes[l][i].width > found_idx) {
            found = true;
            out << "ll" << l << '[' << found_idx/BITS << ']';
            unsigned shift(found_idx%BITS);
            if (shift) out << ">>" << shift;
            out << ")&1";
	  }
          idx += cnodes[l][i].width;
        }
      }
    }
    out << ");\n";
  }
  out << '\n';

  // Copy next register values into registers.
  for (auto ni : ll_idx[0]) {
    unsigned i(ni.second);
    nodeid_t d(static_cast<regimpl*>(nodes[ni.first])->d);
    unsigned j = ll_idx[max_ll][d];
    cnode_t &m(cnodes[max_ll][j]);
    if (m.node == d) {
      out << "    ll0[" << i/BITS << "] &= ~" << (1<<(i%BITS)) << ";\n"
          << "    ll0[" << i/BITS << "] |= (((ll" << max_ll << '['
          << j/BITS << "]>>" << j%BITS << ")&1)<<" << i%BITS << ");\n";
    }
  }

  // Loop end
  out << "  }\n\n";


  // Final code.
  print_c_boilerplate_bottom(out);

  // For debugging, print the evaluation order.
  for (auto v : cnodes) {
    for (auto n : v) {
      if (n.type == DUMMY) cout << '-';
      else                 cout << n.node;
      if (n.type != WIRE && n.type != DUMMY) {
        for (unsigned i = 0; i < nodes[n.node]->src.size(); ++i) {
          if (i == 0) cout << '(';
          cout << nodes[n.node]->src[i];
          if (i == nodes[n.node]->src.size()-1) cout << ')';
          else cout << ',';
        }
      }
      if (n.width != 1) cout << '[' << n.width << ']';
      cout << ' ';
    }
    cout << endl;
  }
}

void print_c_boilerplate_top(ostream &out) {
  out << "#include <stdio.h>\n"
      << "#include <string.h>\n"
      << "#include <stdlib.h>\n"
      << "#include <stdint.h>\n"
      << "#include <sys/time.h> /* For gettimeofday() */\n\n"
      << "unsigned long time_us() {\n"
      << "  struct timeval tv;\n"
      << "  gettimeofday(&tv, NULL);\n"
      << "  return tv.tv_sec * 1000000ul + tv.tv_usec;\n"
      << "}\n\n"
      << "int main(int argc, char** argv) {\n";
}

void print_c_boilerplate_bottom(ostream &out) {
  out << "  fprintf(stderr, \"%fms\\n\", (time_us() - start_time)/1000.0);\n\n"
      << "  return 0;\n"
      << "}\n";
}
