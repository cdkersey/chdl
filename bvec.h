// A bvec is a fixed-length vector of bits. rvecs are like bvecs with a connect
// function.
#ifndef __BVEC_H
#define __BVEC_H

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

  template <unsigned N> class rvec;

  template <unsigned N, typename T> class vec {
    public:
      vec() {}
      vec(const T &r) { for (unsigned i = 0; i < N; ++i) nodes[i] = r; }

      vec &operator=(const vec& r) {
        for (unsigned i = 0; i < N; ++i) nodes[i] = r[i];
        return *this;
      }

      // Indexing operators
      T &operator[](size_t i) { return nodes[i]; }
      const T &operator[](size_t i) const { return nodes[i]; }
      template <unsigned A, unsigned B>
        vec<B-A+1, T> operator[](range<A, B> r)
      {
        vec<B-A+1, T> out;

        for (unsigned i = 0; i < B-A+1; ++i)
          out[i] = (*this)[A+i];

        return out;
      }

    protected:
      T nodes[N];
  };

  template <unsigned N> using bvec = vec<N, node>;

  // Ideally, this would inherit from vec<N, reg>, but then type conversions
  // for arithmetic and logic operators would get weird.
  template <unsigned N> class rvec : public vec<N, node> {
  public:
    rvec() {}
    rvec(const rvec &r) { *this = r; }
    rvec &operator=(const rvec& r) {
      for (unsigned i = 0; i < N; ++i) vec<N, node>::nodes[i] = r[i];
      return *this;
    }

    // Automatic conversion to/from bvec
    rvec(const bvec<N> &r) {
      for (unsigned i = 0; i < N; ++i) vec<N, node>::nodes[i] = r[i];
    }

    operator bvec<N>() const {
      bvec<N> bv;
      for (unsigned i = 0; i < N; ++i) bv[i] = vec<N, node>::nodes[i];
      return bv;
    }
    

    // Indexing operators
    reg &operator[](size_t i) {
      return static_cast<reg&>(vec<N, node>::nodes[i]);
    }
    const reg &operator[](size_t i) const {
      return static_cast<const reg&>(vec<N, node>::nodes[i]);
    }
    template <unsigned A, unsigned B>
      vec<B-A+1, node> operator[](range<A, B> r)
    {
      vec<B-A+1, node> out;
      for (unsigned i = 0; i < B-A+1; ++i)
        out[i] = (*this)[i+A];
      return out;
    }

    // Connect to the D signal.
    void connect(const bvec<N> &bv) {
      for (unsigned i = 0; i < N; ++i)
        static_cast<reg>(vec<N, node>::nodes[i]).connect(bv[i]);
    }
  };
};

#endif
