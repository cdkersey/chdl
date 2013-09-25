// This is the CHDL programmer facing side of the optimization layer. It is used
// solely for the purpose of invoking the optimization layer.
#include "opt.h"
#include "tap.h"
#include "gates.h"
#include "nodeimpl.h"
#include "gatesimpl.h"
#include "litimpl.h"
#include "netlist.h"
#include "regimpl.h"
#include "lit.h"
#include "node.h"
#include "memory.h"
#include "submodule.h"

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
  get_module_inputs(live_nodes);
  get_module_outputs(live_nodes); // TODO: fix this; not necessarily live.
  
  size_t prev_count;
  do {
    prev_count = live_nodes.size();

    // Mark all of the source nodes for each already-marked node
    for (auto n : live_nodes) {
      vector<node> &s(nodes[n]->src);
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
      hpath_t hp(nodes[i]->path);
      litimpl *l0, *l1;
      invimpl *inv = dynamic_cast<invimpl*>(nodes[i]);
      
      if (inv && (l0 = dynamic_cast<litimpl*>(nodes[inv->src[0]]))) {
        ++changes;
        nodes[i] = new litimpl(!l0->eval());
        nodes[i]->path = hp;
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
        nodes[i]->path = hp;
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
        nodes[i]->path = hp;
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
        nodes[nodeid_t(n)]->path = hp;
        continue; 
      }

      if (nand && nand->src[0] == nand->src[1]) {
        node n(i), m(nand->src[0]);
        n = Inv(m);
        nodes[nodeid_t(n)]->path = hp;
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

void chdl::opt_limit_fanout(unsigned max) {
  // This is currently constant, but there should maybe be a way to configure
  // it. The idea is that we don't want to trade high-fanout registers for a
  // ton of clock load. Both are bad.
  bool buffers_for_regs(true);

  // Compute fanouts as outputs to gates + outputs to regs + external outputs
  map<nodeid_t, unsigned> fanout;
  map<nodeid_t, vector<nodeid_t> > succ;
  for (auto p : nodes) {
    for (auto s : p->src) {
      succ[s].push_back(p->id);
      ++fanout[s];
    }
    if (regimpl *r = dynamic_cast<regimpl*>(p)) {
      ++fanout[r->d];
      succ[r->d].push_back(p->id);
    }
  }

  cout << "--- Before ---\n";
  map<unsigned, unsigned> hist;
  for (auto x : fanout) ++hist[x.second];
  for (auto x : hist) cout << "fanout " << x.first << ": " << x.second << endl;

  bool node_split(false);
  do {
    // Find all of the nodes that need to be split
    vector<nodeid_t> nodes_to_split;
    for (auto x : fanout)
      if (x.second > max)
        nodes_to_split.push_back(x.first);    
    if (nodes_to_split.size() > 0) node_split = true;
    else break;

    vector<vector<nodeid_t> > splitnode_suc;
    for (auto x : nodes_to_split) {
      splitnode_suc.push_back(vector<nodeid_t>());
      for (auto y : succ[x]) splitnode_suc.rbegin()->push_back(y);
    }

    // Split them
    for (size_t i = 0; i < nodes_to_split.size(); ++i) {
      nodeid_t id(nodes_to_split[i]);
      nodeimpl *p(nodes[id]);
      vector<nodeid_t> &s(splitnode_suc[i]);

      node new_node;
      regimpl *ri;
      if (nandimpl* ni = dynamic_cast<nandimpl*>(p))
        new_node = Nand(p->src[0], p->src[1]);
      else if (invimpl* ii = dynamic_cast<invimpl*>(p))
        new_node = Inv(p->src[0]);
      else if (!buffers_for_regs && (ri = dynamic_cast<regimpl*>(p)))
        new_node = Reg(ri->d);
      else if (litimpl* li = dynamic_cast<litimpl*>(p))
        new_node = Lit(p->eval());
      else {
        node n(id);
        node intermediate(Inv(id)), repl_node(Inv(intermediate));
        new_node = Inv(intermediate);
        for (unsigned i = s.size()/2; i < s.size(); ++i) {
          nodeid_t sid(s[i]);
          nodeimpl *sp(nodes[sid]);

          if (regimpl* ri = dynamic_cast<regimpl*>(sp)) {
            ri->d.change_net(repl_node);
          } else {
            for (auto &s : sp->src)
              if (s == id) s.change_net(repl_node);
          }
        }
      }

      // Move half of the successors to the new node.
      for (unsigned i = 0; i < s.size()/2; ++i) {
        nodeid_t sid(s[i]);
        nodeimpl *sp(nodes[sid]);

        if (regimpl* ri = dynamic_cast<regimpl*>(sp)) {
          ri->d.change_net(new_node);
        } else {
          for (auto &s : sp->src)
            if (s == id) s.change_net(new_node);
        }
      }
    }

    // Clean up
    opt_dead_node_elimination();

    // Recompute fanout and successors
    succ.clear();
    fanout.clear();
    for (auto p : nodes) {
      for (auto s : p->src) {
        succ[s].push_back(p->id);
        ++fanout[s];
      }
      if (regimpl *r = dynamic_cast<regimpl*>(p)) {
        ++fanout[r->d];
        succ[r->d].push_back(p->id);
      }
    }

  } while (node_split);

  cout << "--- After ---\n";
  hist.clear();
  for (auto x : fanout) ++hist[x.second];
  for (auto x : hist) cout << "fanout " << x.first << ": " << x.second << endl;
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
