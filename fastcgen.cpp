#include <random>
#include <algorithm>

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

const unsigned BITS(32);

void print_c_boilerplate_top(ostream &out);
void print_c_boilerplate_bottom(ostream &out);
// Compute "logic layer" of each node, its distance from the farthest register
// or literal. This is used to determine the order in which node values are
// computed.
void layerize(map<nodeid_t, int> &ll, int &max_ll) {
  set<nodeid_t> regs;
  get_reg_nodes(regs);
  max_ll = 0;
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
}

// The internal representation for the logic.
enum nodetype_t { NAND, INV, REG };
struct cnode_t {
  cnode_t() : type(INV), node(0), width(1) {}
  nodetype_t type;
  nodeid_t node;
  unsigned width;
  vector<pair<int, unsigned> > src;

  bool operator<(const cnode_t &r) const {
    if (int(type) > int(r.type)) return false;
    if (int(type) < int(r.type)) return true;

    for (unsigned i = 0; i < src.size(); ++i) {
      if (i >= r.src.size()) break;

      if (src[i] > r.src[i]) return false;
      if (src[i] < r.src[i]) return true;
    }
    
    if (node > r.node) return false;
    if (node < r.node) return true;

    return false;
  }
};

void gen_ir(vector<vector<cnode_t>> &cnodes, int max_ll,
            map<nodeid_t, int> ll, multimap<int, nodeid_t> ll_r)
{
  cnodes.resize(max_ll + 1);

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
      cn.type = REG;
    }
  }
}

void gen_idx(map<nodeid_t, pair<int, unsigned>> &ll_idx,
             vector<vector<cnode_t>> &cnodes)
{
  ll_idx.clear();

  // Create a way to quickly look up node indices
  for (unsigned i = 0; i < cnodes.size(); ++i)
    for (unsigned j = 0; j < cnodes[i].size(); ++j)
      ll_idx[cnodes[i][j].node] = make_pair(i, j);

  // Set up src vectors
  for (unsigned i = 1; i < cnodes.size(); ++i) {
    for (unsigned j = 0; j < cnodes[i].size(); ++j) {
      cnodes[i][j].src.clear();

      nodeid_t n(cnodes[i][j].node);
      for (unsigned k = 0; k < nodes[n]->src.size(); ++k) {
        cnodes[i][j].src.push_back(ll_idx[nodes[n]->src[k]]);
      }
    }
  }
}

void chdl::print_c(ostream &out) {
  // // // Generate internal representation // // //
  map<nodeid_t, int> ll;
  int max_ll;
  multimap<int, nodeid_t> ll_r;

  layerize(ll, max_ll);

  for (auto l : ll) ll_r.insert(pair<int, nodeid_t>(l.second, l.first));

  // Generate the internal representation of the gates
  vector<vector<cnode_t>> cnodes;
  gen_ir(cnodes, max_ll, ll, ll_r);

  map<nodeid_t, pair<int, unsigned>> ll_idx;

  gen_idx(ll_idx, cnodes);

  // // // Optimize // // //
  for (unsigned l = 0; l < cnodes.size(); ++l)
    sort(cnodes[l].begin(), cnodes[l].end());
    // shuffle(cnodes[l].begin(), cnodes[l].end(), default_random_engine(l));

  gen_idx(ll_idx, cnodes);

  // Set width field appropriately.
  for (unsigned i = 0; i < cnodes.size(); ++i) {
    unsigned input_lvl(i == 0 ? cnodes.size() - 1 : i-1);
    for (unsigned j = 0; j < cnodes[i].size() - 1; j += cnodes[i][j].width) {
      nodetype_t t(cnodes[i][j].type);
      for (unsigned k = j+1; k < cnodes[i].size(); ++k, ++cnodes[i][j].width) {
        if (cnodes[i][j].width == BITS) break;
        if (cnodes[i][k].type != t) break;

        if (i == 0) continue;

        if (t == NAND) {
          pair<int, unsigned> i00(cnodes[i][k-1].src[0]),
	                      i01(cnodes[i][k-1].src[1]),
                              i10(cnodes[i][k].src[0]),
                              i11(cnodes[i][k].src[1]);

          if (i00.first != i10.first) break;
          if (i01.second != i11.second) break;
          if (i00.second + 1 != i10.second) break;
          if (i01.second + 1 != i11.second) break;
        } else if (t == INV) {
          pair<int, unsigned> i00(cnodes[i][k-1].src[0]),
                              i10(cnodes[i][k].src[0]);

          if (i00.first != i10.first) break;
          if (i00.second + 1 != i10.second) break;
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
    for (unsigned i = 0; i < sz; ++i) {
      out << "ll" << l << '_' << i;
      if (l == 0) out << " = 0";
      if (l != cnodes.size() - 1 || i != sz - 1) out << ",\n           ";
    }
  }
  out << ";\n"
      << "  unsigned long cycle;\n\n"
      << "  unsigned long start_time = time_us();\n\n";

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
        unsigned idx(cnodes[l][i].src[k].second),
                 lvl(cnodes[l][i].src[k].first);
        if (k == 0) out << "uint" << BITS << "_t ";
        out << 'i' << k << " = ";
        for (unsigned i = idx; i < idx + w;) {
          unsigned chunk(BITS - i%BITS), remaining_bits(w - (i - idx));
          if (chunk > remaining_bits) chunk = remaining_bits;

          if (i != idx) out << " | ";

          out << "(((ll" << lvl << '_' << i/BITS;
          if (i%BITS) out << ">>" << i%BITS << "ull";
          out << ')';

          if (chunk < BITS) out << '&' << ((1ull<<chunk)-1ull) << "ull";
          out << ')';
          if (i - idx > 0) out << "<<" << i - idx << "ull";
          out << ')';
          i += chunk;
        }
        out << ", ";
      }

      // Perform computation
      out << "v = ";
      if (cnodes[l][i].type == INV) {
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
          out << "ll" << l << '_' << i/BITS << " = v; ";
        } else {
          out << "ll" << l << '_' << i/BITS << " &= ~"
              << (((1ull<<chunk)-1ull)<<(i%BITS)) << "ull; "
              << "ll" << l << '_' << i/BITS << " |= (v";
          if (i - cur_idx > 0) out << ">>" << i - cur_idx << "ull";
          out << ')';
          if (i%BITS) out << " << " << i%BITS << "ull";
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

      unsigned found_idx(ll_idx[n].second), l(ll_idx[n].first);
      out << "ll" << l << '_' << found_idx/BITS;
      unsigned shift(found_idx%BITS);
      if (shift) out << ">>" << shift;
      out << ")&1";
    }
    out << ");\n";
  }
  out << '\n';

  // Copy next register values into registers.
  for (auto ni : ll_idx) {
    if (ni.second.first != 0) continue;
    unsigned i(ni.second.second);
    nodeid_t d(static_cast<regimpl*>(nodes[ni.first])->d);
    unsigned j(ll_idx[d].second), l(ll_idx[d].first);
    out << "    ll0_" << i/BITS << " &= ~" << (1<<(i%BITS)) << "ull;\n"
        << "    ll0_" << i/BITS << " |= (((ll" << l << '_'
        << j/BITS << ">>" << j%BITS << ")&1)<<" << i%BITS << ");\n";
  }

  // Loop end
  out << "  }\n\n";


  // Final code.
  print_c_boilerplate_bottom(out);

  // For debugging, print the evaluation order.
  for (auto v : cnodes) {
    for (auto n : v) {
      cout << n.node;
      for (unsigned i = 0; i < nodes[n.node]->src.size(); ++i) {
        if (i == 0) cout << '(';
        cout << nodes[n.node]->src[i];
        if (i == nodes[n.node]->src.size()-1) cout << ')';
        else cout << ',';
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
