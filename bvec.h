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
  template <unsigned LEN, unsigned A> struct xrange{};

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

      vec(const vec &v) {
        nodes.reserve(N);
        for (unsigned i = 0; i < N; ++i) nodes.push_back(v[i]);
      }

      vec(vec &v) {
        nodes.reserve(N);
        for (unsigned i = 0; i < N; ++i) nodes.push_back(v[i]);
      }

      vec(std::initializer_list<T> l) {
        nodes.reserve(N);
        unsigned i(0);
        for (auto &x : l) { nodes.push_back(x); ++i; }
        for (; i < N; ++i) nodes.push_back(T());
      }

      template <unsigned A> vec(const vec<A, T> &a, const vec<N-A, T> &b) {
        nodes.reserve(N);
        for (unsigned i = 0; i < N-A; ++i) nodes.push_back(b[i]);
        for (unsigned i = 0; i < A; ++i) nodes.push_back(a[i]);
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

      template <unsigned A> vec<0, T> operator[](const xrange<0, A> &r) const {
        return vec<0, T>();
      }

      template <unsigned A> vec<1, T> operator[](const xrange<1, A> &r) const {
	bc(A);
        return vec<1, T>(nodes[A]);
      }

      template <unsigned A, unsigned LEN>
        vec<LEN, T> operator[](const xrange<LEN, A> &r) const
      {
        return vec<LEN, T>((*this)[xrange<LEN-LEN/2,A+LEN/2>()],
                           (*this)[xrange<LEN/2,A>()]);

      }

      template <unsigned A, unsigned B>
        vec<B-A+1, T> operator[](const range<A, B> &r) const
      {
        return (*this)[xrange<B-A+1, A>()];
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
