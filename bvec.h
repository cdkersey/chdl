// A bvec is a fixed-length vector of bits. rvecs are like bvecs with a connect
// function.
#ifndef __BVEC_H
#define __BVEC_H

#include "reg.h"
#include "lit.h"
#include "node.h"
#include "gateops.h"

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

  // The vecref is what we use to assign values to portions of vecs.
  template <unsigned N, typename T> class vecref {
  public:
    vecref(T *base): bp(base) {}

    operator typename chdl::vec<N, T>();

    // This is the whole point of vecrefs. They are either used to assign
    // values to ranges within vecs or they are converted to vecs themselves.
    template <class U>
      vecref &operator=(const U &r)
    {
      for (unsigned i = 0; i < N; ++i) bp[i] = r[i];
      return *this;
    }

    // Don't forget to override the default assignment operator!
    vecref &operator=(const vecref &r) {
      return vecref::operator=<vecref>(r);
    }

    T &operator[](size_t i) const { return bp[i]; }

  private:
    T *bp;
  };

  template <unsigned N> using bvecref = vecref<N, node>;

  template <unsigned N> class rvec;

  template <unsigned N, typename T> class vec {
    public:
      vec() {}
      vec(const T &r) { for (unsigned i = 0; i < N; ++i) nodes[i] = r; }
      vec(const vecref<N, T> &r) { *this = r; }

      vec &operator=(const vec& r) {
        for (unsigned i = 0; i < N; ++i) nodes[i] = r[i];
        return *this;
      }

      vec &operator=(const vecref<N, T>& r) {
        for (unsigned i = 0; i < N; ++i) nodes[i] = r[i];
        return *this;
      }

      // Indexing operators
      T &operator[](size_t i) { return nodes[i]; }
      const T &operator[](size_t i) const { return nodes[i]; }
      template <unsigned A, unsigned B>
        vecref<B-A+1, T> operator[](range<A, B> r)
      {
        return vecref<B-A+1, T>(nodes + A);
      }

    protected:
      T nodes[N];
  };

  template <unsigned N, typename T>
    vecref<N, T>::operator typename chdl::vec<N,T>()
  {
    vec<N, T> r;
    for (unsigned i = 0; i < N; ++i) r[i] = bp[i];
    return r;
  }

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
      vecref<B-A+1, node> operator[](range<A, B> r)
    {
      return vecref<B-A+1, node>(vec<N, node>::nodes + A);
    }

    // Connect to the D signal.
    void connect(const bvec<N> &bv) {
      for (unsigned i = 0; i < N; ++i)
        static_cast<reg>(vec<N, node>::nodes[i]).connect(bv[i]);
    }
  };
};

#endif
