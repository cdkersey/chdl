// A bvec is a fixed-length vector of bits.
#ifndef __BVEC_H
#define __BVEC_H

#include <cstdlib>

#include "reg.h"
#include "lit.h"
#include "node.h"

namespace chdl {
  // Utility function.
  // TODO: put this in (or get this from) a utility library of some sort.
  static unsigned log2(unsigned long n) {
    unsigned i;
    for (i = 0; (1<<i) <= n; ++i);
    return i-1;
  }

  // range is used to index a fixed-width subset of a vec
  template <unsigned A, unsigned B> struct range{};

  template <unsigned N, typename T> class vec;

  template <unsigned N, typename T> class vec {
    public:
      vec() {}
      vec(const T &r) { for (unsigned i = 0; i < N; ++i) nodes[i] = r; }

      vec &operator=(const vec& r) {
        for (unsigned i = 0; i < N; ++i) nodes[i] = r[i];
        return *this;
      }

      // Indexing operators
      T &operator[](size_t i) { bc(i); return nodes[i]; }
      const T &operator[](size_t i) const { bc(i); return nodes[i]; }
      template <unsigned A, unsigned B>
        vec<B-A+1, T> operator[](range<A, B> r) const
      {
        vec<B-A+1, T> out;

        for (unsigned i = 0; i < B-A+1; ++i)
          out[i] = (*this)[A+i];

        return out;
      }

      static constexpr unsigned SZ() { return N * T::SZ(); }

    protected:
      T nodes[N];

    private:
      void bc(size_t i) const { if (i >= N) abort(); }
      void bc(size_t i, size_t j) const { bc(i); bc(j); }
  };

  template <unsigned N> using bvec = vec<N, node>;
};

#endif
