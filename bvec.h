// A bvec is a fixed-length vector of bits.
#ifndef __BVEC_H
#define __BVEC_H

#include <cstdlib>
#include <initializer_list>

#include "reg.h"
#include "lit.h"
#include "node.h"

namespace chdl {
  // range is used to index a fixed-width subset of a vec
  template <unsigned A, unsigned B> struct range{};

  template <unsigned N, typename T> class vec;

  template <unsigned N, typename T> class vec {
    public:
      vec() {}
      vec(const T &r) { for (unsigned i = 0; i < N; ++i) nodes[i] = r; }
      vec(std::initializer_list<T> l) {
        unsigned i(0);
        for (auto &x : l) nodes[i++] = x;
      }

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

    protected:
      T nodes[N];

    private:
      void bc(size_t i) const { if (i >= N) abort(); }
      void bc(size_t i, size_t j) const { bc(i); bc(j); }
  };

  template <unsigned N> using bvec = vec<N, node>;

  // TODO: Standardize this
  template <typename T> struct sz { const static unsigned value = 0; };
  template <> struct sz<node> { const static unsigned value = 1; };

  template <unsigned N, typename T> struct sz<vec<N, T> > {
    const static unsigned value = sz<T>::value * N;
  };
};

#endif
