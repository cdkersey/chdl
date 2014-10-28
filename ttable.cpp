#include <cstdlib>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <random>

#include "tap.h"
#include "gateops.h"
#include "bvec-basic-op.h"

#include "ttable.h"

using namespace chdl;
using namespace std;

static node OrN(const std::vector<node> &x) {
  if (x.size() == 0) return Lit(0);
  else if (x.size() == 1) return x[0];
  
  vector<node> a(x.size()/2), b(x.size() - x.size()/2);
  for (unsigned i = 0; i < x.size()/2; ++i)
    a[i] = x[i];

  for (unsigned i = 0; i < x.size() - x.size()/2; ++i)
    b[i] = x[x.size()/2 + i];

  return OrN(a) || OrN(b);
}

static node AndN(const std::vector<node> &x) {
  if (x.size() == 0) return Lit(1);
  else if (x.size() == 1) return x[0];
  
  vector<node> a(x.size()/2), b(x.size() - x.size()/2);
  for (unsigned i = 0; i < x.size()/2; ++i)
    a[i] = x[i];

  for (unsigned i = 0; i < x.size() - x.size()/2; ++i)
    b[i] = x[x.size()/2 + i];

  return AndN(a) && AndN(b);
}

static node Implicant(const std::vector<node> &in, const std::set<int> &imp) {
  using namespace std;

  vector<node> v(imp.size());

  unsigned i = 0;
  for (auto l : imp) {
    if (l < 0) v[i] = !in[~l];
    else       v[i] = in[l];
    ++i;
  }

  return AndN(v);
}

node Build(const std::vector<node> &in, const std::set<std::set<int> > &imps,
           std::map<std::set<int>, node> &cache)
{
  using namespace std;

  vector<node> anded_imps(imps.size());
  unsigned i = 0;
  for (auto &imp : imps) {
    if (cache.count(imp))
      anded_imps[i] = cache[imp];
    else
      cache[imp] = anded_imps[i] = Implicant(in, imp);
    ++i;
  }

  return OrN(anded_imps);
}

void initial_imps(set<set<int> > &imps, const char *tt, unsigned inputs) {
  for (unsigned i = 0; i < (1<<inputs); ++i) {
    if (tt[i] == '1') {
      set<int> imp;
      for (unsigned j = 0; j < inputs; ++j)
        if (i&(1<<j)) imp.insert(j);
        else imp.insert(~j);
      imps.insert(imp); 
    }
  }
}

void show(const std::set<std::set<int> > &s) {
  for (auto &x : s) {
    cout << "Implicant:";
    for (auto l :x) {
      cout << ' ' << l;
    }
    cout << endl;
  }
  cout << endl;
}

template <typename T, typename F>
  void for_each_shuf(const std::set<T> &s, F func)
{
  using namespace std;

  static auto rng(default_random_engine(time(NULL)));

  std::vector<T> v;
  for (auto &x : s) v.push_back(x);
  shuffle(v.begin(), v.end(), rng);

  for (unsigned i = 0; i < v.size(); ++i)
    func(v[i]);
}

template <typename T>
void for_set_bits(const std::set<int> &imp, int inputs, T func)
{
  using namespace std;

  unsigned idx(0);
  std::vector<int> free_bits;
  for (unsigned i = 0; i < inputs; ++i) {
    // Find the free bits
    if (!imp.count(i) && !imp.count(~i)) free_bits.push_back(i);
    // Fix the values of non-free bits
    else if (imp.count(i)) idx |= (1<<i);
  }

  for (unsigned i = 0; i < (1<<free_bits.size()); ++i) {
    unsigned x(idx);
    // Put bits from i in x;
    for (unsigned j = 0; j < free_bits.size(); ++j)
      if (i & (1<<j)) x |= (1<<free_bits[j]);

    func(x);
  }
  
}

bool check(const std::set<int> &imp, const char *tt, unsigned inputs) {
  using namespace std;

  bool fail(false);

  for_set_bits(
    imp, inputs, [tt, &fail](int x){ if (tt[x] == '0') fail = true; }
  );

  return !fail;
}

// Try to reduce imp by removing literal l, then make sure it still conforms to
// truth table tt.
bool try_reduce(std::set<int> &imp, int l, const char *tt, unsigned inputs)
{
  bool neg(imp.count(~l));
  if (neg) imp.erase(~l);
  else     imp.erase(l);

  // If it doesn't work, put it back.
  if (!check(imp, tt, inputs)) {
    imp.insert(neg ? ~l : l);
    return false;
  }

  return true;
}

bool reduce(std::set<std::set<int> > &imps, const char *tt, unsigned inputs) {
  unsigned count = 0, reductions;
  do {
    reductions = 0;
    set<set<int> > next_imps;
    for (auto &s : imps) {
      set<int> t(s);
      for_each_shuf(s, [&](int x) {
        int l = x < 0 ? ~x : x;
        if (try_reduce(t, l, tt, inputs)) ++reductions;
      });
      next_imps.insert(t);
    }
    imps = next_imps;
    count += reductions;
  } while (reductions);

  return count;
}

bool eliminate_redundancy
  (std::set<std::set<int> > &imps, const char *tt, unsigned inputs) 
{
  using namespace std;

  set<set<int> > new_imps;

  // Count of number of inputs covering each entry in the truth table
  vector<int> icount(1<<inputs);
  for (auto &imp : imps)
    for_set_bits(imp, inputs, [&icount](int x) { ++icount[x]; });

  unsigned rcount, total_r(0);
  do {
    rcount = 0;
    for_each_shuf(imps, [&](set<int> imp) {
      bool redundant = true;
      for_set_bits(imp, inputs, [&icount, &redundant](int x) { 
        if (icount[x] == 1) redundant = false; 
      } );

      if (redundant) {
        ++rcount;
        for_set_bits(imp, inputs, [&icount](int x){ --icount[x]; } );
      } else {
        new_imps.insert(imp);
      }
    });
    imps = new_imps;
    total_r += rcount;
  } while (rcount);

  // cout << "Found " << total_r << " redundant implicants.\n";

  return total_r;
}

node chdl::TruthTable(const std::vector<node> &in, const char *tt,
                      std::map<set<int>, node> &cache)
{
  using namespace std;
  set<set<int> > imps;

  initial_imps(imps, tt, in.size());         // Build initial set of implicants

  reduce(imps, tt, in.size());               // Try to remove literals
  eliminate_redundancy(imps, tt, in.size()); // Remove redundant implicants

  // show(imps);

  // Build the result
  return Build(in, imps, cache);
}
