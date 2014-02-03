// The functions defined in this header are used to do simple analyses on the
// design, such as critical path analysis and gate counts.
#ifndef __ANALYSIS_H
#define __ANALYSIS_H

#include <set>
#include <iostream>

#include "bvec-basic-op.h"
#include "nodeimpl.h"

namespace chdl {
  template <unsigned A, unsigned B> unsigned pathlen(bvec<A> av, bvec<B> bv);
  template <unsigned B> unsigned pathlen(node a, bvec<B> bv);
  template <unsigned A> unsigned pathlen(bvec<A> av, node b);

  size_t num_nands();
  size_t num_inverters();
  size_t num_regs();
  size_t num_sram_bits();

  unsigned critpath();
  bool cycdet();

  void critpath_report(std::ostream &);
};

template <unsigned A, unsigned B>
  unsigned chdl::pathlen(chdl::bvec<A> av, chdl::bvec<B> bv)
{
  using namespace chdl;
  using namespace std;
 
  set<nodeid_t> a, b;
  for (unsigned i = 0; i < A; ++i) a.insert(av[i]);
  for (unsigned i = 0; i < B; ++i) b.insert(bv[i]);

  unsigned max_l(0);

  for (auto n : b) {
    unsigned l(0), l_b(0);
    set<nodeid_t> frontier;
    frontier.insert(n);

    while (!frontier.empty()) {
      ++l;

      set<nodeid_t> next_frontier;
      for (auto n : frontier) {
        if (a.find(n) != a.end()) l_b = l;
        for (unsigned i = 0; i < nodes[n]->src.size(); ++i)
          next_frontier.insert(nodes[n]->src[i]);
      }

      frontier = next_frontier;
    }
    if (l_b > max_l) max_l = l_b;
  }

  return max_l;
}

template <unsigned B> unsigned chdl::pathlen(chdl::node a, chdl::bvec<B> bv) {
  return pathlen(chdl::bvec<1>(a), bv);
}

template <unsigned A> unsigned chdl::pathlen(chdl::bvec<A> av, chdl::node b) {
  return pathlen(av, chdl::bvec<1>(b));
}

#endif
