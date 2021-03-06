// A bvec is a fixed-length vector of bits.
#ifndef __BVEC_BASIC_H
#define __BVEC_BASIC_H

#include "bvec.h"
#include "gates.h"
#include "hierarchy.h"

namespace chdl {
  // Concatenate two vectors
  template <typename T, unsigned A, unsigned B>
    vec<A + B, T> Cat(const vec<A, T> &a, const vec<B, T> &b)
  {
    return vec<A + B, T>(a, b);
  }

  // Concatenate a vector and a single element
  template <typename T, unsigned A>
    vec<A + 1, T> Cat(const vec<A, T> &a, const T &b)
  {
    return Cat(a, vec<1, T>(b));
  }

  template <typename T, unsigned B>
    vec<B + 1, T> Cat(const T &a, const vec<B, T> &b)
  {
    return Cat(vec<1, T>(a), b);
  }

  static bvec<1> Flatten(const node &n) { return bvec<1>(n); }
  static bvec<1> Flatten(const bvec<1> &n) { return n; }

  template <typename T>
    bvec<0> Flatten(const vec<0, T> &x)
  {
    return bvec<0>();
  }

  template <typename T>
    bvec<sz<T>::value> Flatten(const vec<1, T> &x)
  {
    return Flatten(x[0]);
  }

  template <unsigned N, typename T>
    bvec<N * sz<T>::value> Flatten(const vec<N, T> &x)
  {
    return vec<N*sz<T>::value, node>(
      Flatten(x[range<N/2,N-1>()]),
      Flatten(x[range<0,N/2-1>()])
    );
  }

  static inline bvec<2> Cat(const node &a, const node &b) {
    return Cat(bvec<1>(a), bvec<1>(b));
  }

  template <unsigned N> struct concatenator : public bvec<N> {
    concatenator(const bvec<N> &x): bvec<N>(x) {}

    template <unsigned M> concatenator<N + M> Cat(const bvec<M> &x) {
      return concatenator<N + M>(chdl::Cat(*(bvec<N>*)this, x));
    }

    concatenator <N + 1> Cat(node x) {
      return concatenator<N + 1>(chdl::Cat(*(bvec<N>*)this, x));
    }
  };

  template <unsigned N> concatenator<N> Cat(const bvec<N> &x) {
    return concatenator<N>(x);
  }

  static concatenator<1> Cat(const node &x) { return concatenator<1>(x); }

  // Create an array of registers.
  template <typename T> T Reg(const T &d, vec<sz<T>::value, bool> val) {
    HIERARCHY_ENTER();
    bvec<sz<T>::value> s;
    T r;
    Flatten(r) = s;
    for (unsigned i = 0; i < sz<T>::value; ++i)
      s[i] = Reg(Flatten(d)[i], val[i]);
    HIERARCHY_EXIT();
    return r;
  }

  template <typename T> T Reg(T d, unsigned long val=0) {
    const unsigned N(sz<T>::value);
    vec<sz<T>::value, bool> x;
    for (unsigned i = 0; i < N; ++i) x[i] = (val>>i)&1;
    return Reg(d, x); 
  }

  // Add a write signal to an existing array of registers
  template <typename T>
    void Wreg(T &q, const T &d, node w, unsigned long val=0)
  {
    HIERARCHY_ENTER();
    q = Reg(Mux(w, q, d), val);
    HIERARCHY_EXIT();
  }

  template <typename T>
    void Wreg(T &q, const T &d, node w, vec<sz<T>::value, bool> val)
  {
    HIERARCHY_ENTER();
    q = Reg(Mux(w, q, d), val);
    HIERARCHY_EXIT();
  }

  // Create an array of registers with a "write" signal
  template <typename T>
    T Wreg(node w, T d, unsigned long val=0)
  {
    T r;
    Wreg(r, d, w, val);
    return r;
  }

  template <typename T>
    T Wreg(node w, T d, vec<sz<T>::value, bool> val)
  {
    T r;
    Wreg(r, d, w, val);
    return r;
  }

  // Create a binary integer literal with the given value
  template <unsigned N> bvec<N> Lit(unsigned long long val) {
    HIERARCHY_ENTER();
    bvec<N> r;
    for (size_t i = 0; i < N; ++i) r[i] = Lit((val>>i)&1);
    HIERARCHY_EXIT();
    return r;
  }

  // Zero-extend (or truncate if output is smaller)
  template <unsigned N, unsigned M> bvec<N> Zext(bvec<M> x) {
    HIERARCHY_ENTER();
    bvec<N> rval;
    if (M >= N) {
      for (unsigned i = 0; i < N; ++i) rval[i] = x[i];
    } else {
      for (unsigned i = 0; i < M; ++i) rval[i] = x[i];
      for (unsigned i = M; i < N; ++i) rval[i] = Lit(0);
    }
    HIERARCHY_EXIT();

    return rval;
  }

  // Sign-extend (or truncate if output is smaller)
  template <unsigned N, unsigned M> bvec<N> Sext(bvec<M> x) {
    HIERARCHY_ENTER();
    bvec<N> rval;
    if (M >= N) {
      for (unsigned i = 0; i < N; ++i) rval[i] = x[i];
    } else {
      for (unsigned i = 0; i < M; ++i) rval[i] = x[i];
      for (unsigned i = M; i < N; ++i) rval[i] = x[M-1];
    }
    HIERARCHY_EXIT();

    return rval;
  }

  // Perform an operation element-wise over an N-bit array.
  template <node (*op)(const node &, const node &)>
    bvec<0> OpElementwise(const bvec<0> &a, const bvec<0> &b)
  {
    return bvec<0>();
  }

  template <node (*op)(const node &, const node &)>
    bvec<1> OpElementwise(const bvec<1> &a, const bvec<1> &b)
  {
    return bvec<1>{op(a[0], b[0])};
  }

  template <node (*op)(const node &, const node &), unsigned N>
    bvec<N> OpElementwise(const bvec<N> &a, const bvec<N> &b)
  {
    return Cat(OpElementwise<op>(a[range<N/2,N-1>()], b[range<N/2,N-1>()]),
               OpElementwise<op>(a[range<0,N/2-1>()], b[range<0,N/2-1>()]));
  }

  // Perform an all-reduce type operation over the given operation to produce
  // a 1-bit result. Used to, say, construct N-input gates from a 2-input op.
  template <node (*OP)(const node &, const node &), bool I>
    node OpReduce(const bvec<0> &in)
  { return Lit(I); }
  
  template <node (*op)(const node &, const node &), bool I>
    node OpReduce(const bvec<1> &in)
  { return in[0]; }

  template <node (*op)(const node &, const node &), bool I, unsigned N>
    node OpReduce(bvec<N> in)
  {
    return op(OpReduce<op, I>(in[range<N/2,N-1>()]),
              OpReduce<op, I>(in[range<0,N/2-1>()]));
  }

  // Some common operations in element-wise form
  static bvec<0> Not(const bvec<0> &in) { return bvec<0>(); }
  static bvec<1> Not(const bvec<1> &in) { return bvec<1>(!in[0]); }

  template <unsigned N>
    bvec<N> Not(const bvec<N> &in)
  {
      HIERARCHY_ENTER();
      bvec<N> r(Cat(Not(in[range<N/2,N-1>()]), Not(in[range<0,N/2-1>()])));
      HIERARCHY_EXIT();

      return r; 
  }

  template <unsigned N> bvec<N> And(const bvec<N> &a, const bvec<N> &b) {
    HIERARCHY_ENTER();
    bvec<N> r(OpElementwise<And>(a, b));
    HIERARCHY_EXIT();
    return r;
  }

  template <unsigned N> bvec<N> Or (const bvec<N> &a, const bvec<N> &b) {
    HIERARCHY_ENTER();
    bvec<N> r(OpElementwise<Or>(a, b));
    HIERARCHY_EXIT();
    return r;
  }

  template <unsigned N> bvec<N> Xor(const bvec<N> &a, const bvec<N> &b) {
    HIERARCHY_ENTER();
    bvec<N> r(OpElementwise<Xor>(a, b));
    HIERARCHY_EXIT();
    return r;
  }

  // Those same operations in all-reduce form
  template <unsigned N> node AndN(const bvec<N> &in) {
    HIERARCHY_ENTER();
    node r(OpReduce<And, 1>(in));
    HIERARCHY_EXIT();
    return r;
  }

  template <unsigned N> node OrN (const bvec<N> &in) {
    HIERARCHY_ENTER();
    node r(OpReduce< Or, 0>(in));
    HIERARCHY_EXIT();
    return r;
  }

  template <unsigned N> node XorN(const bvec<N> &in) {
    HIERARCHY_ENTER();
    node r(OpReduce< Xor, 0>(in));
    HIERARCHY_EXIT();
    return r;
  }

  // Detect whether two values are equal
  template <unsigned N> node EqDetect(const bvec<N> &a, const bvec<N> &b) {
    HIERARCHY_ENTER();
    node r(AndN(Not(Xor(a, b))));
    HIERARCHY_EXIT();
    return r;
  }
};

#endif
