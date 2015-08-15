#include <iostream>
#include <fstream>
#include <algorithm>
#include <set>
#include <vector>
#include <list>

#include "techmap.h"

#include "gates.h"
#include "reg.h"
#include "tap.h"
#include "nodeimpl.h"
#include "regimpl.h"
#include "gatesimpl.h"
#include "input.h"
#include "trisimpl.h"
#include "tristate.h"
#include "litimpl.h"
#include "opt.h"
#include "memory.h"

using namespace std;
using namespace chdl;

// In a tri-state world a node can be driven by multiple gates. This _only_
// happens with the tri-state node, but now it's possible to say something like
// "node 5 gate 3".
unsigned nGates(nodeid_t n) {
  tristateimpl *t(dynamic_cast<tristateimpl*>(nodes[n]));
  if (t) return t->src.size()/2;
  else return 1;
}

struct mapping {
  mapping(): cd(0) {}

  int t, cd;
  nodeid_t output;
  map<char, nodeid_t> input;
  set<nodeid_t> covered;
};

struct tlibgate {
  tlibgate();
  tlibgate(string s, int *idx = NULL);
  tlibgate(const tlibgate&);
  ~tlibgate();

  tlibgate &operator=(const tlibgate &r);

  int get_size() const;

  bool match(nodeid_t n, int g, mapping &m);

  string dump();

  enum gtype { INV, REG, NAND, TRISTATE, INPUT, HIGH, LOW };
  bool seq, aliased;
  gtype t;
  char c, alias;
  tlibgate *i0, *i1;
};

tlibgate::tlibgate(): seq(false), aliased(false), i0(NULL), i1(NULL), c('x'), t(INPUT) {}

tlibgate::tlibgate(string s, int *idx): seq(false), aliased(false) {
  if (s[0] == '=') {
    aliased = true;
    alias = s[1];
    s = s.substr(2);
  }

  if (s.find("r") != string::npos) seq = true;
  c = s[0];

  if (s[0] == 'i') {
    // INV
    t = INV;
    i0 = new tlibgate(s.substr(1), idx);
    i1 = NULL;
  } else if (s[0] == 'r') {
    // REG
    t = REG;
    i0 = new tlibgate(s.substr(1), idx);
    i1 = NULL;
  } else if (s[0] == 'n') {
    int n(0);

    // NAND
    t = NAND;
    i0 = new tlibgate(s.substr(1), &n);
    i1 = new tlibgate(s.substr(1 + n), idx);
    if (idx) *idx += n;
  } else if (s[0] == 't') {
    // TRISTATE

    int n(0);

    t = TRISTATE;
    i0 = new tlibgate(s.substr(1), &n);
    i1 = new tlibgate(s.substr(1 + n), idx);
  } else if (s[0] == 'H') {
    // HIGH
    t = HIGH;
    i0 = i1 = NULL;
  } else if (s[0] == 'L') {
    // LOW
    t = LOW;
    i0 = i1 = NULL;
  } else { 
    // INPUT
    t = INPUT;
    i0 = i1 = NULL;
  }

  if (i0 && i0->seq) seq = true;
  if (i1 && i1->seq) seq = true;

  if (idx) ++*idx;
}

tlibgate::tlibgate(const tlibgate &r):
  t(r.t), c(r.c), seq(r.seq), aliased(r.aliased), alias(r.alias)
{
  if (r.i0) i0 = new tlibgate(*r.i0); else i0 = NULL;
  if (r.i1) i1 = new tlibgate(*r.i1); else i1 = NULL;
}

tlibgate &tlibgate::operator=(const tlibgate &r) {
  t = r.t;
  c = r.c;
  seq = r.seq;
  aliased = r.aliased;
  alias = r.alias;
  if (i0) delete i0;
  if (i1) delete i1;
  if (r.i0) i0 = new tlibgate(*r.i0); else i0 = NULL;
  if (r.i1) i1 = new tlibgate(*r.i1); else i1 = NULL;
  return *this;
}

tlibgate::~tlibgate() {
  if (i0) delete i0;
  if (i1) delete i1;
}

int tlibgate::get_size() const {
  if (t == INPUT) return 0;

  int n(1);
  if (i0) n += i0->get_size();
  if (i1) n += i1->get_size();
  return n;
}

string tlibgate::dump() {
  string r;

  if (aliased) {
    r.push_back('=');
    r.push_back(alias);
  }

  if (t == INV) {
    r += "i" + i0->dump();
  } else if (t == REG) {
    r += "r" + i0->dump();
  } else if (t == NAND) {
    r += "n" + i0->dump() + i1->dump();
  } else if (t == TRISTATE) {
    r += "t" + i0->dump() + i1->dump();
  } else if (t == HIGH) {
    r += "H";
  } else if (t == LOW) {
    r += "L";
  } else if (t == INPUT) {
    r.push_back(c);
  }

  return r;
}

bool tlibgate::match(nodeid_t n, int g, mapping &m) {
  bool rval(false);

  map<char, nodeid_t> bak(m.input);
  vector<nodeid_t> v;
  if (t != INPUT) v.push_back(n);

  bool alias_fail = false;
  if (aliased) {
    if (m.input.count(alias)) {
      if (n != m.input[alias])
        alias_fail = true;
    } else {
      m.input[alias] = n;
    }
  }

  if (!alias_fail) {
    // Inputs are on the boundaries of components and match everything.
    if (t == INPUT) {
      if (m.input.count(c)) {
        rval = (m.input[c] == n);
      } else {
        m.input[c] = n;
        rval = true;
      }
    } else if (t == INV) {
      invimpl *p(dynamic_cast<invimpl*>(nodes[n]));
      if (!p) rval = false;
      else    rval = i0->match(p->src[0], -1, m);
    } else if (t == REG) {
      regimpl *p(dynamic_cast<regimpl*>(nodes[n]));
      if (!p) {
        rval = false;
      } else {
        m.cd = p->cd;
        rval = i0->match(p->d, -1, m);
      }
    } else if (t == TRISTATE) {
      if (g == -1) rval = false;
      else {
        tristateimpl *p(dynamic_cast<tristateimpl*>(nodes[n]));
        if (!p)
          rval = false;
        else
          rval = i0->match(p->src[2*g], -1, m)
  	    && i1->match(p->src[2*g + 1], -1, m);
      }
    } else if (t == NAND) {
      nandimpl *p(dynamic_cast<nandimpl*>(nodes[n]));
      if (!p) rval = false;
      else {
        map<char, nodeid_t> bak(m.input);
        rval = (i0->match(p->src[0], -1, m) && i1->match(p->src[1], -1, m));
        if (!rval) {
          m.input = bak;
          rval = (i0->match(p->src[1], -1, m) && i1->match(p->src[0], -1, m));
        }
      }
    } else if (t == HIGH || t == LOW) {
      litimpl *p(dynamic_cast<litimpl*>(nodes[n]));
      rval = (p && p->eval(0) == (t == HIGH));
    }

    if (rval) for (auto x : v) m.covered.insert(x);
  }

  if (!rval) m.input = bak;
  
  return rval;
}

bool operator<(const tlibgate &l, const tlibgate &r) {
  return l.get_size() < r.get_size();
}

bool no_internal_outputs(const set<nodeid_t> &nodes, nodeid_t output,
                         map<nodeid_t, set<nodeid_t>> &users)
{
  for (auto n : nodes)
    for (auto &u : users[n])
      if (n != output && nodes.find(u) == nodes.end()) return false;

  return true;
}




// Creates a map mapping nodes n to their successor set sm[n], including those
// successors which are registers.
void get_successors(map<nodeid_t, set<nodeid_t>> &sm) {
  sm.clear();

  for (nodeid_t n = 0; n < nodes.size(); ++n) {
    nodeimpl *p(nodes[n]);

    if (regimpl *r = dynamic_cast<regimpl*>(p)) sm[r->d].insert(n);
    else for (auto s : p->src) sm[s].insert(n);
  }
}

void insert_reg_inverters(set<int> inv, map<nodeid_t, set<nodeid_t>> &sm) {
  if (inv.empty()) return;

  nodeid_t nsize(nodes.size());
  for (nodeid_t n = 0; n < nsize; ++n) {
    nodeid_t id(n);
    if (!dynamic_cast<regimpl*>(nodes[n])) continue;

    // First, replace registers themselves with Inv(Inv(reg))
    if (inv.count(-1)) {
      regimpl *r = dynamic_cast<regimpl*>(nodes[n]);
      // Don't do this if all the successors to our register are inverters.
      bool invSucc(true);
      for (auto s : sm[n]) {
        invimpl *ii(dynamic_cast<invimpl*>(nodes[s]));
        if (!ii) invSucc = false;
      }
      if (!invSucc) {
        node rn(id);
        node new_reg(Reg(r->d));
        id = nodes.size() - 1;
        rn = Inv(Inv(new_reg));
        // BUG/TODO: We should annihilate the original register, because it
        // will otherwise remain in the design post techmapping.
      }
    }

    // Next, replace inputs x with Inv(Inv(x))
    if (inv.count(0)) {
      regimpl *r = dynamic_cast<regimpl*>(nodes[id]);
      // If the predecessor node is already an inverter, ignore it.
      invimpl *ii(dynamic_cast<invimpl*>(nodes[r->d]));
      if (ii) continue;

      node rn(id);
      rn = Reg(Inv(Inv(r->d)));
      // BUG/TODO: (see prev.)
    }
  }
}

void insert_tris_inverters(set<int> inv, map<nodeid_t, set<nodeid_t>>&) {
  if (inv.empty()) return;

  // Replace inputs x with Inv(Inv(x))
  if (inv.count(0) || inv.count(1)) {
    nodeid_t nsize(nodes.size());
    for (nodeid_t n = 0; n < nsize; ++n) {
      if (tristateimpl *ti = dynamic_cast<tristateimpl*>(nodes[n])) {
        tristatenode tn(n), new_tn;
        for (unsigned i = 0; i < ti->src.size(); i += 2) {
          // If the predecessor node is already an inverter, don't add any.
          bool inv0(inv.count(0)), inv1(inv.count(1));
          invimpl *ii(dynamic_cast<invimpl*>(nodes[ti->src[i]]));
          if (ii) inv0 = false;
          ii = dynamic_cast<invimpl*>(nodes[ti->src[i + 1]]);
          if (ii) inv1 = false;

          new_tn.connect(
            (inv0 ? Inv(Inv(ti->src[i])) : ti->src[i]),
            (inv1 ? Inv(Inv(ti->src[i + 1])) : ti->src[i + 1]) 
          );          
        }
        tn = new_tn;
      }
    }
  }
}

void insert_nand_inverters(set<int> inv, map<nodeid_t, set<nodeid_t>> &sm) {
  if (inv.empty()) return;

  nodeid_t nsize(nodes.size());
  for (nodeid_t n = 0; n < nsize; ++n) {
    nodeid_t id(n);
    if (!dynamic_cast<nandimpl*>(nodes[id])) continue;

    // First, replace nands themselves with Inv(Inv(nand))
    if (inv.count(-1)) {
      // Don't do this if all the successors to our gate are inverters.
      bool invSucc(true);
      for (auto s : sm[n]) {
        invimpl *ii(dynamic_cast<invimpl*>(nodes[s]));
        if (!ii) invSucc = false;
      }
      if (!invSucc) {
        node nn(id);
        node newnand(Nand(nodes[id]->src[0], nodes[id]->src[1]));
        id = nodes.size() - 1;
        nn = Inv(Inv(newnand));
      }
    }

    // Next, replace inputs x with Inv(Inv(x))
    if (inv.count(0) || inv.count(1)) {
      // If the predecessor node is already an inverter, ignore it.
      //invimpl *ii(dynamic_cast<invimpl*>(nodes[nodes[id]->src[0]]));
      //if (ii || !inv.count(0)) {
      //  ii = dynamic_cast<invimpl*>(nodes[nodes[id]->src[1]]);
      //  if (ii || !inv.count(1)) continue;
      //}

      node nn(id);
      nn = Nand(
        (inv.count(0) ? Inv(Inv(nodes[id]->src[0])) : nodes[id]->src[0]),
        (inv.count(1) ? Inv(Inv(nodes[id]->src[1])) : nodes[id]->src[1])
      );
    }
  }
}

// Sometimes there are node types that do not map directly to anything in the
// tech library, and logic must be added so that a technology mapping solution
// can be found. As an example, the tri-state buffer in FreePDK45 is inverting,
// and with SPST relays, and gates are a fundamental type but nand gates are
// not.
void addInverters(vector<pair<tlibgate, string>> &tlib) {
  // These are the types that we must be able to cover. We assume the tech
  // library contains an inverter or there's no guarantee technology mapping
  // will work.
  set<tlibgate::gtype> gt{tlibgate::NAND, tlibgate::TRISTATE, tlibgate::REG};

  // If a gate type appears in "bare" form in tlib, remove it from the list
  for (auto g : gt) {
    for (auto &x : tlib) {
      tlibgate *tg(&x.first);
      if (tg->t == g && tg->i0->t == tlibgate::INPUT && 
           (tg->t == tlibgate::REG || tg->i1->t == tlibgate::INPUT))
      {
        cout << "Type " << g << " bare. Removing from invgates.\n";
        gt.erase(g);
      } 
    }
  }

  // Where should inverters be added?
  map<tlibgate::gtype, set<int> > inv; // -1 for output, 1, 2 for inputs
  for (auto g : gt) {
    for (auto &x : tlib) {
      bool inv_input(false);
      tlibgate *tg(&x.first);
      if (tg->t == tlibgate::INV) { inv_input = true; tg = tg->i0; }
      if (tg->t != g) continue;
      if (tg->i0->t != tlibgate::INV && tg->i0->t != tlibgate::INPUT) continue; 
      if (tg->i0->t==tlibgate::INV && tg->i0->i0->t!=tlibgate::INPUT) continue;
      if (tg->t != tlibgate::REG) {
        if (tg->i1->t!=tlibgate::INV && tg->i1->t!=tlibgate::INPUT) continue;
        if (tg->i1->t==tlibgate::INV&&tg->i1->i0->t!=tlibgate::INPUT) continue;
      }

      if (inv_input) inv[g].insert(-1);
      if (tg->i0->t == tlibgate::INV) inv[g].insert(0);
      if (tg->t != tlibgate::REG && tg->i1->t==tlibgate::INV) inv[g].insert(1);

      for (auto x : inv[g]) cout << "inv " << x << ' ';
      cout << endl;
      break;
    }

    map<nodeid_t, set<nodeid_t>> sm;
    get_successors(sm);

    if (!inv[g].empty()) {
      cout << "Inserting inverters on " << g << endl;
      if (g == tlibgate::REG)
        insert_reg_inverters(inv[tlibgate::REG], sm);
      else if (g == tlibgate::NAND)
        insert_nand_inverters(inv[tlibgate::NAND], sm);
      else if (g == tlibgate::TRISTATE)
        insert_tris_inverters(inv[tlibgate::TRISTATE], sm);
    }

    opt_dead_node_elimination();
  }

}

void chdl::techmap(ostream &out, const char* tlibFile) {
  using namespace std;
  using namespace chdl;

  const bool ALLOW_REP(true);

  vector<pair<tlibgate, string>> tlib;

  // Read technology library
  ifstream tf(tlibFile);
  for (;;) {
    string name, desc;
    tf >> name >> desc;
    if (!tf) break;
    tlibgate t(desc);
    tlib.push_back(make_pair(t, name));
  }

  sort(tlib.begin(), tlib.end());

  for (auto &t : tlib) cout << t.second << ": " << t.first.dump() << endl;

  addInverters(tlib);

  vector<list<mapping>> bestmaps(nodes.size());

  map<nodeid_t, set<nodeid_t>> users;
  for (unsigned n = 0; n < nodes.size(); ++n) {
    if (regimpl *r = dynamic_cast<regimpl*>(nodes[n])) users[r->d].insert(n);
    for (auto s : nodes[n]->src) users[s].insert(n);
  }

  for (unsigned n = 0; n < nodes.size(); ++n) {
    mapping bestmap;
    bool mapping_found(false);
    for (unsigned g = 0; g < nGates(n); ++g) {
      for (unsigned e_idx = 0; e_idx < tlib.size(); ++e_idx) {
        auto &e(tlib[e_idx]);
        tlibgate &t(e.first);
        mapping m;
        if (t.match(n, g, m)) {
          if (ALLOW_REP || no_internal_outputs(m.covered, n, users)) {
            m.t = e_idx;
            m.output = n;
            bestmap = m;
            mapping_found = true;
          }
        }
      }
      if (mapping_found) bestmaps[n].push_back(bestmap);
    }
  }

  // Print outputs and inputs
  out << "inputs" << endl;
  print_input_nodes(out);
  out << "outputs" << endl;
  print_tap_nodes(out);
  out << "inout" << endl;
  print_io_tap_nodes(out);
  out << "design" << endl;

  // Map outputs and register inputs, then their inputs, etc., until there is
  // nothing left to map.
  set<nodeid_t> nodes_to_map, nodes_mapped;
  get_reg_d_nodes(nodes_to_map);
  get_tap_nodes(nodes_to_map);
  while(!nodes_to_map.empty()) {
    set<nodeid_t> next_nodes;

    for (auto n : nodes_to_map) {
      if (nodes_mapped.count(n)) continue;
      nodes_mapped.insert(n);
      while (!bestmaps[n].empty()) {
        mapping &m(bestmaps[n].front());
        out << "  " << tlib[m.t].second;
	if (m.cd) out << '<' << m.cd << '>';
        for (auto x : m.input) {
          next_nodes.insert(x.second);
          if (x.first >= '0' && x.first <= '9')
            out << ' ' << x.second;
        }
        out << ' ' << n << endl;
        bestmaps[n].pop_front();
      }
    }

    nodes_to_map = next_nodes;
  }

  // Print SRAM arrays
  set<nodeid_t> m;
  get_mem_q_nodes(m);
  for (auto i : m) nodes[i]->print(out);
}
