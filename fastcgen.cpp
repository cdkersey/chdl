#include "analysis.h"
#include "nodeimpl.h"
#include "gatesimpl.h"
#include "regimpl.h"
#include "reg.h"
#include "hierarchy.h"
#include "netlist.h"

using namespace std;
using namespace chdl;

void chdl::print_c(ostream &out) {
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
    nodetype_t type;
    nodeid_t node;
  };

  vector<vector<cnode_t>> cnodes(max_ll+1);
  for (auto x : ll_r) {
    int l(x.first), inputs(0);
    cnodes[l].push_back(cnode_t());
    cnode_t &cn(*cnodes[l].rbegin());

    cn.node = x.second;
    inputs = nodes[x.second]->src.size();
    
    if (nandimpl *p = dynamic_cast<nandimpl*>(nodes[x.second])) {
      cn.type = INV;
    } else if (invimpl *p = dynamic_cast<invimpl*>(nodes[x.second])) {
      cn.type = NAND;
    }

    for (unsigned i = 0; i < inputs; ++i) {
      nodeid_t n(nodes[x.second]->src[i]);
      if (ll.find(n) == ll.end()) {
        cerr << "Node " << n << " has no logic level.\n";
      }
      int i_ll(ll[n]);
      for (int j = l - 1; j > i_ll; --j) {
        cnodes[j].push_back(cnode_t());
        cnode_t &icn(*cnodes[j].rbegin());
        icn.type = WIRE;
        icn.node = n;
      }
    }
  }

  for (auto r : cnodes[0]) {
    regimpl *ri = dynamic_cast<regimpl*>(nodes[r.node]);
    nodeid_t d(ri->d);
    if (ll.find(d) == ll.end())
      cerr << "Node " << d << " has no logic level.\n";
    int d_ll(ll[d]);
    for (int j = max_ll; j > d_ll; --j) {
      cnodes[j].push_back(cnode_t());
      cnode_t &dcn(*cnodes[j].rbegin());
      dcn.type = WIRE;
      dcn.node = d;
    }
  }

  // For debugging, print the evaluation order.
  for (auto v : cnodes) {
    for (auto n : v) {
      cout << n.node;
      if (n.type != WIRE) {
        for (unsigned i = 0; i < nodes[n.node]->src.size(); ++i) {
          if (i == 0) cout << '(';
          cout << nodes[n.node]->src[i];
          if (i == nodes[n.node]->src.size()-1) cout << ')';
          else cout << ',';
        }
      }
      cout << ' ';
    }
    cout << endl;
  }
}
