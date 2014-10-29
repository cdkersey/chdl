#include "analysis.h"
#include "opt.h"
#include "tap.h"
#include "input.h"
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
#include "trisimpl.h"
#include "tristate.h"

#include <vector>
#include <set>
#include <map>

using namespace chdl;
using namespace std;

void chdl::opt_set_dontcare() {
  // Find dontcare nodes
  set<nodeid_t> d;
  for (nodeid_t n = 0; n < nodes.size(); ++n)
    if (litimpl *l = dynamic_cast<litimpl*>(nodes[n]))
      if (l->is_undef()) d.insert(n); 

  // Find successors of all nodes.
  map<nodeid_t, set<nodeid_t> > succ;
  for (auto n : nodes) for (auto s : n->src) succ[s].insert(n->id);

  // +1 for each nand after even number of inverters, -1 for each nand after odd
  // number of inverters.
  map<nodeid_t, int> score;

  // Assuming there is a tree of inverters from each dontcare literal, this is
  // used to follow the tree in a breadth-first manner.
  map<nodeid_t, set<nodeid_t> > invs;
  for (auto n : d) invs[n].insert(n); // To bootstrap the process.
  
  // TODO: This needs to be tested.
  bool descended;
  int layer(0);
  do {
    descended = false;
    for (auto n : d) {
      set<nodeid_t> next_invs;
      for (auto x : invs[n]) {
        for (auto s : succ[x]) {
          if (dynamic_cast<nandimpl*>(nodes[s])) {
            if (layer%2) ++score[n];
            else --score[n];
          } else if (dynamic_cast<invimpl*>(nodes[s])) {
            next_invs.insert(s);
            descended = true;
          }
        }
      }
      invs[n] = next_invs;
    }

    ++layer;
  } while(descended);

  for (auto s : score) cout << s.first << ": " << s.second << endl;

  for (auto n : d) static_cast<litimpl*>(nodes[n])->resolve(score[n] > 0);
 
}

bool is_or(nodeid_t n, nodeid_t &in0, nodeid_t &in1) {
  if (dynamic_cast<nandimpl*>(nodes[n]) &&
      dynamic_cast<invimpl*>(nodes[nodes[n]->src[0]]) &&
      dynamic_cast<invimpl*>(nodes[nodes[n]->src[1]]))
  {
    in0 = nodes[nodes[n]->src[0]]->src[0];
    in1 = nodes[nodes[n]->src[1]]->src[0];

    return true;
  }

  return false;
}

bool is_and(nodeid_t n, nodeid_t &in0, nodeid_t &in1) {
  if (dynamic_cast<invimpl*>(nodes[n]) &&
      dynamic_cast<nandimpl*>(nodes[nodes[n]->src[0]]))
  {
    in0 = nodes[nodes[n]->src[0]]->src[0];
    in1 = nodes[nodes[n]->src[0]]->src[1];

    return true;
  }
}

node gen_or_blob_v(const vector<nodeid_t> &in, const hpath_t &path) {
  if (in.size() == 0) abort(); // assert(in.size() != 0);
  if (in.size() == 1) return node(in[0]);

  vector<nodeid_t> a, b;

  for (unsigned i = 0; i < in.size(); i += 2) a.push_back(in[i]);
  for (unsigned i = 1; i < in.size(); i += 2) b.push_back(in[i]);

  node x = Inv(gen_or_blob_v(a, path));
  nodes[nodes.size() - 1]->path = path;
  node y = Inv(gen_or_blob_v(b, path));
  nodes[nodes.size() - 1]->path = path;
  node out = Nand(x, y);
  nodes[nodes.size() - 1]->path = path;

  return out;
}

node gen_and_blob_v(const vector<nodeid_t> &in, const hpath_t &path) {
  if (in.size() == 0) abort(); // assert(in.size() != 0);
  if (in.size() == 1) return node(in[0]);

  vector<nodeid_t> a, b;

  for (unsigned i = 0; i < in.size(); i += 2) a.push_back(in[i]);
  for (unsigned i = 1; i < in.size(); i += 2) b.push_back(in[i]);

  node x = Nand(gen_and_blob_v(a, path), gen_and_blob_v(b, path));
  nodes[nodes.size() - 1]->path = path;
  node out = Inv(x);
  nodes[nodes.size() - 1]->path = path;

  return out;
}

node gen_or_blob(const set<nodeid_t> &in, const hpath_t &path) {
  vector<nodeid_t> in_v;
  for (auto n : in) in_v.push_back(n);
  return gen_or_blob_v(in_v, path);
}

node gen_and_blob(const set<nodeid_t> &in, const hpath_t &path) {
  vector<nodeid_t> in_v;
  for (auto n : in) in_v.push_back(n);
  return gen_and_blob_v(in_v, path);
}

template <typename T, typename U>
  void assoc_balance_internal(T &is_x, U &gen)
{
  map<nodeid_t, set<nodeid_t> > blobs, blobgates, succ;
  set<nodeid_t> tapnodes, regnodes;
  get_tap_nodes(tapnodes);
  get_reg_nodes(regnodes);
  for (nodeid_t n = 0; n < nodes.size(); ++n)
    for (auto s : nodes[n]->src)
      succ[s].insert(n);

  // Find initial set of 2-input gates
  for (nodeid_t i = 0; i < nodes.size(); ++i) {
    nodeid_t a, b;
    if (is_x(i, a, b)) {
      blobs[i].insert(a);
      blobs[i].insert(b);
      blobgates[i].insert(i);
    }
  }

  // Merge new gates into existing blobs until there are none left to merge.
  bool merge;
  do {
    merge = false;
    for (auto &b : blobs) {
      for (auto i : b.second) {
        if (blobs.count(i)) {
          bool externalSuccessors(false);
          if (tapnodes.count(i) || regnodes.count(i)) externalSuccessors = true;
          if (succ[i].size() > 1) {
            externalSuccessors = true;
          }
          if (externalSuccessors) continue;
          
          blobgates.erase(i);
          blobgates[b.first].insert(i);
          for (auto j : blobs[i]) b.second.insert(j);
          b.second.erase(i);
          blobs.erase(i);
          merge = true;
          goto next;
        }
      }
    }
    next:;
  } while(merge);

  // Replace existing or-blobs with new, balanced or-blobs
  for (auto &x : blobs) {
    if (x.second.size() < 4) continue;
    node n(x.first);
    hpath_t path = nodes[x.first]->path;
    n = gen(x.second, path);
  }
}

void chdl::opt_assoc_balance() {
  assoc_balance_internal(is_and, gen_and_blob);
  assoc_balance_internal(is_or, gen_or_blob);
}

void chdl::opt_dead_node_elimination() {
  set<nodeid_t> live_nodes;

  // Start with an initial set of live nodes: taps, register D nodes, and memory
  // address and D bits.
  get_mem_nodes(live_nodes);
  get_tap_nodes(live_nodes);
  get_reg_nodes(live_nodes);
  get_input_nodes(live_nodes);
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
    if (live_nodes.count(i)) pm[i] = dest++;
  }
  permute_nodes(pm);
}

void chdl::node_sweep() {
  // Remove any nodeimpl to which no node objects point.
  set<nodeid_t> dead_nodes;
  get_dead_nodes(dead_nodes);

  map<nodeid_t, nodeid_t> pm;
  nodeid_t dest(0);
  for (nodeid_t i = 0; i < nodes.size(); ++i)
    if (dead_nodes.count(i) == 0) pm[i] = dest++;
  permute_nodes(pm);
}

template<typename T> node vecOrN(T begin, T end) {
  unsigned len(end - begin);
  T middle(begin + len/2);
  if (len == 1)
    return *begin;
  else
    return Or(vecOrN(begin, middle), vecOrN(middle, end));
}

template <typename T> node vecOrN(const T &v) {
  return vecOrN(v.begin(), v.end());
}

void safeErase(vector<node> &v, unsigned begin, unsigned end) {
  // vector<T>::resize() apparently also uses T::operator=
  //for (unsigned i = 0; i < end - begin; ++i)
  //  v[begin + i].change_net(v[end + i]);
  //v.resize(v.size() - (end - begin));

  // This one seems to work, but yuck.
  vector<node> w;
  for (unsigned i = 0; i < v.size(); ++i)
    if (i < begin || i >= end)
      w.push_back(v[i]);
  v.clear();
  v = w;
}

void chdl::opt_tristate_merge() {
  map<nodeid_t, map<nodeid_t, vector<nodeid_t> > > tris;

  for (nodeid_t i = 0; i < nodes.size(); ++i) {
    if (tristateimpl *t = dynamic_cast<tristateimpl*>(nodes[i])) {
      map<nodeid_t, vector<nodeid_t> > &inputs(tris[i]); // input->enable
      for (unsigned j = 0; j < t->src.size(); j += 2)
        inputs[t->src[j]].push_back(t->src[j+1]);
    }
  }

  for (auto &t : tris) {
    tristatenode nt;
    for (auto &x : t.second)
      nt.connect(x.first, vecOrN(x.second));
    node n(nodes[t.first]->id);
    n = nt;
  }

  opt_dead_node_elimination();
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
        nodes[i] = new litimpl(!l0->eval(0));
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
        ++changes;
        continue; 
      }

      nandimpl *nand = dynamic_cast<nandimpl*>(nodes[i]);
      if (nand && (l0 = dynamic_cast<litimpl*>(nodes[nand->src[0]])) &&
                  (l1 = dynamic_cast<litimpl*>(nodes[nand->src[1]])))
      {
        ++changes;
        nodes[i] = new litimpl(!(l0->eval(0) && l1->eval(0)));
        nodes[i]->path = hp;
        nodes.pop_back();
        nodes[i]->id = i;
        delete nand;
        continue;
      }

      if (nand && (
          (l0=dynamic_cast<litimpl*>(nodes[nand->src[0]])) && !(l0->eval(0)) ||
          (l1=dynamic_cast<litimpl*>(nodes[nand->src[1]])) && !(l1->eval(0))))
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

      tristateimpl *tris = dynamic_cast<tristateimpl*>(nodes[i]);
      if (tris) {
        for (unsigned i = 0; i < tris->src.size(); i += 2) {
          if (litimpl *l = dynamic_cast<litimpl*>(nodes[tris->src[i+1]])) {
            if (l->eval(0)) {
              node n(tris->id);
              n = tris->src[i];
              ++changes;
              break;
            } else {
              safeErase(tris->src, i, i+2);

              if (tris->src.size() == 0) {
                node n(tris->id);
                n = Lit(1);
              }
              ++changes;
              break;
            }
          }
        }
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
      lnode = (l->eval(0))?lit1:lit0;
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
  bool buffers_for_regs(true), buffers_for_all(true);

  // Compute fanouts as outputs to gates + outputs to regs + external outputs
  map<nodeid_t, unsigned> fanout;
  map<nodeid_t, vector<pair<nodeid_t, unsigned>>> succ;
  for (auto p : nodes) {
    for (unsigned i = 0; i < p->src.size(); ++i) {
      if (tristateimpl *ti = dynamic_cast<tristateimpl*>(p)) {
        // Don't count buses.
      } else {
        nodeid_t s = p->src[i];
        succ[s].push_back(pair<nodeid_t, unsigned>(p->id, i));
        ++fanout[s];
      }
    }
    if (regimpl *r = dynamic_cast<regimpl*>(p)) {
      ++fanout[r->d];
      succ[r->d].push_back(pair<nodeid_t, unsigned>(p->id, 0));
    }
  }

  cout << "--- Before ---\n";
  map<unsigned, unsigned> hist;
  for (auto x : fanout) ++hist[x.second];
  for (auto x : hist) cout << "fanout " << x.first << ": " << x.second << endl;

  for (;;) {
    // Find all of the nodes that need to be split
    vector<nodeid_t> nodes_to_split;
    for (auto x : fanout)
      if (x.second > max)
        nodes_to_split.push_back(x.first);    
    if (nodes_to_split.size() == 0) break;

    vector<vector<pair<nodeid_t, unsigned>>> splitnode_suc;
    for (auto x : nodes_to_split) {
      splitnode_suc.push_back(vector<pair<nodeid_t, unsigned>>());
      for (auto y : succ[x]) splitnode_suc.rbegin()->push_back(y);
    }

    // Split them
    for (size_t i = 0; i < nodes_to_split.size(); ++i) {
      nodeid_t id(nodes_to_split[i]);
      nodeimpl *p(nodes[id]);
      vector<pair<nodeid_t, unsigned>> &s(splitnode_suc[i]);

      node new_node;
      regimpl *ri;
      nandimpl *ni;
      invimpl *ii;
      if (!buffers_for_all && (ni = dynamic_cast<nandimpl*>(p)))
        new_node = Nand(p->src[0], p->src[1]);
      else if (!buffers_for_all && (ii = dynamic_cast<invimpl*>(p)))
        new_node = Inv(p->src[0]);
      else if (!buffers_for_regs && (ri = dynamic_cast<regimpl*>(p)))
        new_node = Reg(ri->d);
      else if (litimpl *li = dynamic_cast<litimpl*>(p))
        new_node = Lit(p->eval(0));
      else if (tristateimpl *ti = dynamic_cast<tristateimpl*>(p)) {
        // It's silly to add buffers exiting a bus.
      } else {
        node n(id);
        node intermediate(Inv(id)), repl_node(Inv(intermediate));
        new_node = Inv(intermediate);
        for (unsigned i = s.size()/2; i < s.size(); ++i) {
          nodeid_t sid(s[i].first);
          unsigned pos(s[i].second);
          nodeimpl *sp(nodes[sid]);

          if (regimpl* ri = dynamic_cast<regimpl*>(sp))
            ri->d.change_net(repl_node);
          else
            sp->src[pos].change_net(repl_node);
        }
      }

      // Move half of the successors to the new node.
      for (unsigned i = 0; i < s.size()/2; ++i) {
        nodeid_t sid(s[i].first);
        unsigned pos(s[i].second);
        nodeimpl *sp(nodes[sid]);

        if (regimpl* ri = dynamic_cast<regimpl*>(sp))
          ri->d.change_net(new_node);
        else
          sp->src[pos].change_net(new_node);
      }
    }

    // Clean up
    opt_dead_node_elimination();

    // Recompute fanout and successors
    // TODO: remove this copypaste
    succ.clear();
    fanout.clear();
    for (auto p : nodes) {
      for (unsigned i = 0; i < p->src.size(); ++i) {
        if (tristateimpl *ti = dynamic_cast<tristateimpl*>(p)) {
          // Don't count buses
        } else {
          nodeid_t s = p->src[i];
          succ[s].push_back(pair<nodeid_t, unsigned>(p->id, i));
          ++fanout[s];
        }
      }
      if (regimpl *r = dynamic_cast<regimpl*>(p)) {
        ++fanout[r->d];
        succ[r->d].push_back(pair<nodeid_t, unsigned>(p->id, 0));
      }
    }

  }

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

  opt_tristate_merge();
  opt_contract();
  opt_dedup();
  opt_tristate_merge();
  cerr << "After tri-state merge: " << nodes.size() << endl;
}
