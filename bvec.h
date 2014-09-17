// A bvec is a fixed-length vector of bits.
#ifndef __BVEC_H
#define __BVEC_H

#include <cstdlib>
#include <initializer_list>
#include <vector>

#include "reg.h"
#include "lit.h"
#include "node.h"

namespace chdl {
  // range is used to index a fixed-width subset of a vec
  template <unsigned A, unsigned B> struct range{};

  template <unsigned N, typename T> class vec;

  template <unsigned N, typename T> class vec {
    public:
      virtual ~vec() {}
      vec(): nodes(N) {}
      vec(const T &r) {
        nodes.reserve(N);
        for (unsigned i = 0; i < N; ++i) nodes.push_back(r);
      }

      template <typename U> vec(const vec<N, U> &v) {
        nodes.reserve(N);
        for (unsigned i = 0; i < N; ++i) nodes.push_back(v[i]);
      }

      vec(std::initializer_list<T> l) {
        nodes.reserve(N);
        unsigned i(0);
        for (auto &x : l) { nodes.push_back(x); ++i; }
        for (; i < N; ++i) nodes.push_back(node());
      }

      template <unsigned A> vec(const vec<A, T> &a, const vec<N-A, T> &b) {
        nodes.reserve(N);
        for (unsigned i = 0; i < A; ++i) nodes.push_back(a[i]);
        for (unsigned i = A; i < N; ++i) nodes.push_back(b[i - A]);
      }

      vec &operator=(const vec &r) {
        for (unsigned i = 0; i < N; ++i) nodes[i] = r[i];
        return *this;
      }

      template <typename U> vec &operator=(const vec<N, U> &r) {
        for (unsigned i = 0; i < N; ++i) nodes[i] = r[i];
        return *this;
      }

      // Indexing operators
      typename std::vector<T>::reference operator[](size_t i) {
        bc(i); return nodes[i];
      }

      typename std::vector<T>::const_reference operator[](size_t i) const {
        bc(i); return nodes[i];
      }

      template <unsigned A, unsigned B>
        vec<B-A+1, T> operator[](range<A, B> r) const
      {
        vec<B-A+1, T> out;

        for (unsigned i = 0; i < B-A+1; ++i)
          out[i] = (*this)[A+i];

        return out;
      }

    protected:
      std::vector<T> nodes;

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
