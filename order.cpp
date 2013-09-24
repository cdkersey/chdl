#include "node.h"
#include "opt.h"
#include "nodeimpl.h"
#include "regimpl.h"

#include <climits>
#include <vector>
#include <map>
#include <unordered_map>
#include <list>
#include <unordered_set>
#include <iostream>
#include <random>
#include <algorithm>
#include <limits>

using namespace std;
using namespace chdl;

unsigned SEED(0x1234);
default_random_engine rando(SEED);

struct edge: public pair<nodeid_t, nodeid_t> {
  edge(nodeid_t a, nodeid_t b): pair<nodeid_t, nodeid_t>(a>b?b:a, a>b?a:b) {
    // assert(a != b);
  }
};


ostream &operator<<(ostream &o, const edge &e) {
  o << '(' << e.first << ", " << e.second << ')';
  return o;
}

static void find_neighbors(unordered_map<nodeid_t, vector<nodeid_t> > &m) {
  for (auto p : nodes) {
    if (auto r = dynamic_cast<regimpl*>(p)) {
      if (nodeid_t(r->d) != r->id) {
        m[r->d].push_back(r->id);
        m[r->id].push_back(r->d);
      }
    }

    for (auto s : p->src) {
      m[p->id].push_back(s);
      m[s].push_back(p->id);
    }
  }
}

static double sa_temp(unsigned step, unsigned steps) {
  double t(double(step + 1)/steps);
  t = 1000000 * pow(0.9, 100*t);
  return t;
}

static unsigned sa_len(nodeid_t n, nodeid_t m,
                unordered_map<nodeid_t, int> &edgepos,
                unordered_map<nodeid_t, vector<nodeid_t> > &neighbors)
{
  unsigned len(0);
  for (auto x : neighbors[n])
    if (x != m) len += abs(edgepos[x] - edgepos[n]);

  return len;
}

static unsigned total_len(unordered_map<nodeid_t, int> &edgepos,
                   unordered_map<nodeid_t, vector<nodeid_t> > &neighbors)
{
  unsigned total(0);
  for (nodeid_t i = 0; i < nodes.size(); ++i)
    for (auto n : neighbors[i])
      total += abs(edgepos[n] - edgepos[i]);
  cout << "Sum of edge lengths: " << total/2.0 << endl;
  return total / 2;
}

static void sa_verify(vector<nodeid_t> o) {
  unordered_map<nodeid_t, int> edgepos;
  unordered_map<nodeid_t, vector<nodeid_t> > neighbors;
 
  // Create a reverse lookup (nodeid -> edge position in o)
  for (nodeid_t i = 0; i < o.size(); ++i) edgepos[o[i]] = i;

  find_neighbors(neighbors);

  // Find length of every edge in the design
  total_len(edgepos, neighbors);
}

void chdl::order(vector<nodeid_t> &o, unsigned steps) {
  o.clear();
  if (steps == 0) steps = 10 * nodes.size() * nodes.size();

  unordered_map<nodeid_t, int> edgepos;
  unordered_map<nodeid_t, vector<nodeid_t> > neighbors;

  // Create an initial randomized set
  for (nodeid_t i = 0; i < nodes.size(); ++i) o.push_back(i);
  shuffle(o.begin(), o.end(), rando);

  // Create a reverse lookup (nodeid -> edge position in o)
  for (nodeid_t i = 0; i < o.size(); ++i) edgepos[o[i]] = i;

  find_neighbors(neighbors);

  // Find length of every edge in the design
  total_len(edgepos, neighbors);

  int gainsum(0);
  for (unsigned step = 0; step < steps; ++step) {
    uniform_int_distribution<> da(0, o.size()-1), db(0,o.size()-1);
    unsigned a_idx(da(rando)), b_idx(db(rando));
    nodeid_t na(o[a_idx]), nb(o[b_idx]);

    int l0(sa_len(na, nb, edgepos, neighbors) +
           sa_len(nb, na, edgepos, neighbors));

    swap(edgepos[na], edgepos[nb]);
    swap(o[a_idx], o[b_idx]);

    int l1(sa_len(na, nb, edgepos, neighbors) +
           sa_len(nb, na, edgepos, neighbors)); 

    uniform_real_distribution<> d(0,1.0);
    if (l1 > l0 && d(rando) > exp(-(l1-l0)/sa_temp(step, steps))) {
      swap(edgepos[na], edgepos[nb]);
      swap(o[a_idx], o[b_idx]);
    } else {
      gainsum += l0 - l1;
    }

    //cout << "gainsum = " << gainsum << endl;
  }

  total_len(edgepos, neighbors);
}
