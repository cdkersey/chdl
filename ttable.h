#ifndef CHDL_TTABLE_H
#define CHDL_TTABLE_H

#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <random>

namespace chdl {

node TruthTable(const std::vector<node> &in, const char *tt,
                std::map<std::set<int>, node> &cache);

template <unsigned N>
node TruthTable(const bvec<N> &in, const char *tt,
                std::map<std::set<int>, node> &cache)
{
  using namespace std;

  vector<node> v(N);
  for (unsigned i = 0; i < N; ++i) v[i] = in[i];

  return TruthTable(v, tt, cache);
}

template <unsigned A>
node LLRom(const bvec<A> &a, const std::vector<bool> &contents,
            std::map<std::set<int>, node> &cache)
{
  char tt[1ul<<A];
  unsigned sz(contents.size() > (1ul<<A) ? (1ul<<A) : contents.size());

  for (unsigned i = 0; i < sz; ++i) {
    if (contents[i]) tt[i] = '1';
    else             tt[i] = '0';
  }

  for (unsigned i = sz; i < (1ul<<A); ++i) {
    tt[i] = 'x';
  }

  return TruthTable(a, tt, cache);
}

// TODO: LLRom<A, N, T> for arbitrary vectors of T

template <unsigned A, unsigned N>
  bvec<N> LLRom(const bvec<A> &a, const char *filename)
{
  using namespace std;

  ifstream in(filename);
  vector<unsigned long> contents;

  while (!!in) {
    unsigned long x;
    in >> hex >> x;
    if (!!in) contents.push_back(x);
  }

  bvec<N> q;
  map<set<int>, node> cache;

  for (unsigned i = 0; i < N; ++i) {
    vector<bool> tt;
    for (unsigned j = 0; j < contents.size(); ++j)
      tt.push_back((contents[j] >> i) & 1);
    q[i] = NewRom(a, tt, cache);
  }

  return q;
}

}

#endif
