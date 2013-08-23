#include <iostream>
#include <fstream>
#include <algorithm>
#include <set>
#include <vector>

#include "techmap.h"

#include "reg.h"
#include "tap.h"
#include "nodeimpl.h"
#include "regimpl.h"
#include "gatesimpl.h"

using namespace std;
using namespace chdl;

struct mapping {
  int t;
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

  bool match(nodeid_t n, mapping &m);

  string dump();

  enum gtype { INV, REG, NAND, INPUT };
  bool seq;
  gtype t;
  char c;
  tlibgate *i0, *i1;
};

tlibgate::tlibgate(): seq(false), i0(NULL), i1(NULL), c('x'), t(INPUT) {}

tlibgate::tlibgate(string s, int *idx): seq(false) {
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
  } else { 
    // INPUT
    t = INPUT;
    i0 = i1 = NULL;
  }

  if (i0 && i0->seq) seq = true;
  if (i1 && i1->seq) seq = true;

  if (idx) ++*idx;
}

tlibgate::tlibgate(const tlibgate &r): t(r.t), c(r.c), seq(r.seq) {
  if (r.i0) i0 = new tlibgate(*r.i0); else i0 = NULL;
  if (r.i1) i1 = new tlibgate(*r.i1); else i1 = NULL;
}

tlibgate &tlibgate::operator=(const tlibgate &r) {
  t = r.t;
  c = r.c;
  seq = r.seq;
  if (i0) delete i0;
  if (i1) delete i1;
  if (r.i0) i0 = new tlibgate(*r.i0); else i0 = NULL;
  if (r.i1) i1 = new tlibgate(*r.i1); else i1 = NULL;
}

tlibgate::~tlibgate() {
  if (i0) delete i0;
  if (i1) delete i1;
}

int tlibgate::get_size() const {
  int n(1);
  if (i0) n += i0->get_size();
  if (i1) n += i1->get_size();
  return n;
}

string tlibgate::dump() {
  if (t == INV) {
    return "i" + i0->dump();
  } else if (t == REG) {
    return "r" + i0->dump();
  } else if (t == NAND) {
    return "n" + i0->dump() + i1->dump();
  } else if (t == INPUT) {
    string s("x");
    s[0] = c;
    return string(s);
  }
}

bool tlibgate::match(nodeid_t n, mapping &m) {
  bool rval(false);

  map<char, nodeid_t> bak(m.input);
  vector<nodeid_t> v;
  if (t != INPUT) v.push_back(n);

  // Inputs are on the boundaries of components and match everything.
  if (t == INPUT) {
    if (m.input.find(c) != m.input.end()) {
      rval = (m.input[c] == n);
    } else {
      m.input[c] = n;
      rval = true;
    }
  } else if (t == INV) {
    invimpl *p(dynamic_cast<invimpl*>(nodes[n]));
    if (!p) rval = false;
    else    rval = i0->match(p->src[0], m);
  } else if (t == REG) {
    regimpl *p(dynamic_cast<regimpl*>(nodes[n]));
    if (!p) rval = false;
    else    rval = i0->match(p->d, m);
  } else if (t == NAND) {
    nandimpl *p(dynamic_cast<nandimpl*>(nodes[n]));
    if (!p) rval = false;
    else {
      map<char, nodeid_t> bak(m.input);
      rval = (i0->match(p->src[0], m) && i1->match(p->src[1], m));
      if (!rval) {
        m.input = bak;
        rval = (i0->match(p->src[1], m) && i1->match(p->src[0], m));
      }
    }
  }

  if (!rval) m.input = bak;
  else for (auto x : v) m.covered.insert(x);

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

void chdl::techmap(ostream &out) {
  using namespace std;
  using namespace chdl;

  const bool ALLOW_REP(true);

  vector<pair<tlibgate, string>> tlib;

  // Read technology library
  ifstream tf("TLIB");
  for (;;) {
    string name, desc;
    tf >> name >> desc;
    if (!tf) break;
    tlibgate t(desc);
    tlib.push_back(make_pair(t, name));

    cout << name << ": " << t.dump() << '(' << t.get_size() << ')'
         << (t.seq?" [seq]\n":"\n");
  }

  sort(tlib.begin(), tlib.end());

  vector<mapping> bestmaps(nodes.size());

  map<nodeid_t, set<nodeid_t>> users;
  for (unsigned n = 0; n < nodes.size(); ++n) {
    if (regimpl *r = dynamic_cast<regimpl*>(nodes[n])) users[r->d].insert(n);
    for (auto s : nodes[n]->src) users[s].insert(n);
  }

  for (unsigned n = 0; n < nodes.size(); ++n) {
    for (unsigned e_idx = 0; e_idx < tlib.size(); ++e_idx) {
      auto &e(tlib[e_idx]);
      tlibgate &t(e.first);
      mapping m;
      if (t.match(n, m)) {
        if (ALLOW_REP || no_internal_outputs(m.covered, n, users)) {
          m.t = e_idx;
          m.output = n;
          bestmaps[n] = m;
        }
      }
    }
  }

  // Map outputs and register inputs, then their inputs, etc., until there is
  // nothing left to map.
  set<nodeid_t> nodes_to_map, nodes_mapped;
  get_reg_d_nodes(nodes_to_map);
  get_tap_nodes(nodes_to_map);
  out << endl << endl;
  while(!nodes_to_map.empty()) {
    set<nodeid_t> next_nodes;

    for (auto n : nodes_to_map) {
      if (nodes_mapped.find(n) != nodes_mapped.end()) continue;
      nodes_mapped.insert(n);
      mapping &m(bestmaps[n]);
      out << "  " << tlib[m.t].second;
      for (auto x : m.input) {
        if (!tlib[m.t].first.seq) next_nodes.insert(x.second);
        out << ' ' << x.second;
      }
      out << ' ' << n << endl;
    }

    nodes_to_map = next_nodes;
  }
}
