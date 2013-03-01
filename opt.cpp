// This is the CHDL programmer facing side of the optimization layer. It is used
// solely for the purpose of invoking the optimization layer.
#include "opt.h"
#include "tap.h"
#include "gates.h"
#include "nodeimpl.h"
#include "gatesimpl.h"
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

void chdl::opt_dead_node_elimination() {
  set<nodeid_t> live_nodes;

  // Start with an initial set of live nodes: taps, register D nodes, and memory
  // address and D bits.
  get_mem_nodes(live_nodes);
  get_tap_nodes(live_nodes);
  get_reg_nodes(live_nodes);
  
  size_t prev_count;
  do {
    prev_count = live_nodes.size();

    // Mark all of the source nodes for each already-marked node
    for (auto it = live_nodes.begin(); it != live_nodes.end(); ++it) {
      vector<node> &s(nodes[*it]->src);
      for (unsigned i = 0; i < s.size(); ++i) 
        live_nodes.insert(s[i]);
    }
  } while(prev_count != live_nodes.size());

  // Create the permutation map and permute the nodes.
  map<nodeid_t, nodeid_t> pm;
  nodeid_t dest(0);
  for (nodeid_t i = 0; i < nodes.size(); ++i) {
    if (live_nodes.find(i) != live_nodes.end()) pm[i] = dest++;
  }
  permute_nodes(pm);
}

void chdl::opt_contract() {
  unsigned changes;
  do {
    changes = 0;
    for (unsigned i = 0; i < nodes.size(); ++i) {
      litimpl *l0, *l1;
      invimpl *inv = dynamic_cast<invimpl*>(nodes[i]);
      
      if (inv && (l0 = dynamic_cast<litimpl*>(nodes[inv->src[0]]))) {
        ++changes;
        nodes[i] = new litimpl(!l0->eval());
        nodes.pop_back();
        nodes[i]->id = i;
        delete inv;
        continue;
      }

      invimpl *inv2;
      if (inv && (inv2 = dynamic_cast<invimpl*>(nodes[inv->src[0]]))) {
        node n(inv->id), m(inv2->src[0]);
        n = m;
        continue; 
      }

      nandimpl *nand = dynamic_cast<nandimpl*>(nodes[i]);
      if (nand && (l0 = dynamic_cast<litimpl*>(nodes[nand->src[0]])) &&
                  (l1 = dynamic_cast<litimpl*>(nodes[nand->src[1]])))
      {
        ++changes;
        nodes[i] = new litimpl(!(l0->eval() && l1->eval()));
        nodes.pop_back();
        nodes[i]->id = i;
        delete nand;
        continue;
      }

      if (nand && (
          (l0 = dynamic_cast<litimpl*>(nodes[nand->src[0]])) && !(l0->eval()) ||
          (l1 = dynamic_cast<litimpl*>(nodes[nand->src[1]])) && !(l1->eval())))
      {
        ++changes;
        nodes[i] = new litimpl(1);
        nodes.pop_back();
        nodes[i]->id = i;
        delete nand;
        continue;
      }

      if (nand && ((l0 = dynamic_cast<litimpl*>(nodes[nand->src[0]])) || 
                   (l1 = dynamic_cast<litimpl*>(nodes[nand->src[1]]))))
      {
        ++changes;
        node n(i), m(nand->src[l0?1:0]);
        n = Inv(m);
        continue; 
      }

      if (nand && nand->src[0] == nand->src[1]) {
        node n(i), m(nand->src[0]);
        n = Inv(m);
        ++changes;
        continue;
      }
    }

    opt_dead_node_elimination();

  } while (changes);
}

void chdl::opt_combine_literals() {
  size_t n = nodes.size();
  node lit0(Lit(0)), lit1(Lit(1));

  for (size_t i = 0; i < n; ++i) {
    litimpl *l(dynamic_cast<litimpl*>(nodes[i]));
    if (l) {
      node lnode(l->id);
      lnode = (l->eval())?lit1:lit0;
    }
  }

  opt_dead_node_elimination();
}

void chdl::opt_dedup() {
  unsigned changes;
  do {
    map<set<nodeid_t>, nodeid_t> nands;
    map<nodeid_t, nodeid_t> invs;
    changes = 0;
    for (nodeid_t i = 0; i < nodes.size(); ++i) {
      invimpl *inv = dynamic_cast<invimpl*>(nodes[i]);
      nandimpl *nand = dynamic_cast<nandimpl*>(nodes[i]);
      if (inv) {
        nodeid_t input(inv->src[0]);

        auto it(invs.find(input));
        if (it == invs.end()) {
          invs[input] = i;
        } else {
          node n(i), m(invs[input]);
          n = m;
          ++changes;
        }
      } else if (nand) {
        set<nodeid_t> inputs;
        inputs.insert(nand->src[0]);
        inputs.insert(nand->src[1]);
      
        auto it(nands.find(inputs));
        if (it == nands.end()) {
          nands[inputs] = i;
        } else {
          node n(i), m(nands[inputs]);
          n = m;
          ++changes;
        }
      }
    }

    opt_dead_node_elimination();
  } while(changes);
}

void chdl::optimize() {
  cerr << "Before optimization: " << nodes.size() << endl;
  opt_dead_node_elimination();
  cerr << "After dead node elimination: " << nodes.size() << endl;
  opt_contract();
  cerr << "After contraction: " << nodes.size() << endl;
  opt_combine_literals();
  cerr << "After combining literals: " << nodes.size() << endl;
  opt_dedup();
  cerr << "After redundant expression elimination: " << nodes.size() << endl;
}
