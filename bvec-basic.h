// A bvec is a fixed-length vector of bits.
#ifndef __BVEC_BASIC_H
#define __BVEC_BASIC_H

#include "bvec.h"
#include "gates.h"
#include "hierarchy.h"

namespace chdl {
  // Concatenate two bit vectors
  template <unsigned A, unsigned B>
    bvec<A + B> Cat(bvec<A> a, bvec<B> b)
  {
    bvec<A + B> r;
    for (unsigned i = 0; i < B; ++i) r[i] = b[i];
    for (unsigned i = 0; i < A; ++i) r[B+i] = a[i];
    return r;
  }

  // Concatenate a bit vector and a node
  template <unsigned A> bvec<A + 1> Cat(bvec<A> a, node b) {
    return Cat(a, bvec<1>(b));
  }

  template <unsigned B> bvec<B + 1> Cat(node a, bvec<B> b) {
    return Cat(bvec<1>(a), b);
  }

  template <unsigned N, unsigned M>
    bvec<N*M> Flatten(const vec<N, bvec<M> > &x)
  {
    return bvec<N*M>(x);
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
  template <unsigned N> bvec<N> Reg(bvec<N> d, vec<N, bool> val) {
    HIERARCHY_ENTER();
    bvec<N> r;
    for (unsigned i = 0; i < N; ++i)
      r[i] = Reg(d[i], val[i]);
    HIERARCHY_EXIT();
    return r;
  }

  template <unsigned N> bvec<N> Reg(bvec<N> d, unsigned long val=0) {
    vec<N, bool> x;
    for (unsigned i = 0; i < N; ++i) x[N-i-1] = (val>>i)&1;
    return Reg(d, x); 
  }

  // Add a write signal to an existing array of registers
  template <unsigned N>
    void Wreg(bvec<N> q, bvec<N> d, node w, unsigned long val=0)
  {
    HIERARCHY_ENTER();
    for (unsigned i = 0; i < N; ++i)
      q[i] = Reg(Mux(w, q[i], d[i]), val & (1ull<<i));
    HIERARCHY_EXIT();
  }

  template <unsigned N>
    void Wreg(bvec<N> q, bvec<N> d, node w, vec<N, bool> val)
  {
    HIERARCHY_ENTER();
    for (unsigned i = 0; i < N; ++i)
      q[i] = Reg(Mux(w, q[i], d[i]), val[i]);
    HIERARCHY_EXIT();
  }

  // Create an array of registers with a "write" signal
  template <unsigned N>
    bvec<N> Wreg(node w, bvec<N> d, unsigned long val=0)
  {
    bvec<N> r;
    Wreg(r, d, w, val);
    return r;
  }

  template <unsigned N>
    bvec<N> Wreg(node w, bvec<N> d, vec<N, bool> val)
  {
    bvec<N> r;
    Wreg(r, d, w, val);
    return r;
  }

  // Create a binary integer literal with the given value
  template <unsigned N> bvec<N> Lit(unsigned long val) {
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
  template <unsigned N, node (*op)(node, node)>
    bvec<N> OpElementwise(bvec<N> a, bvec<N> b)
  {
    bvec<N> r;
    for (unsigned i = 0; i < N; ++i) r[i] = op(a[i], b[i]);
    return r;
  }

  // Perform an all-reduce type operation over the given operation to produce
  // a 1-bit result. Used to, say, construct N-input gates from a 2-input op.
  template <unsigned N, node (*op)(node, node)>
    node OpReduce(bvec<N> in)
  {
    bvec<2*N> nodes;
    nodes[range<N, 2*N-1>()] = in;
    for (int i = N-1; i >= 0; --i) nodes[i] = op(nodes[2*i], nodes[2*i + 1]);
    return nodes[1];
  }

  // Some common operations in element-wise form
  template <unsigned N>
    bvec<N> Not(bvec<N> in)
  {
      HIERARCHY_ENTER();
      bvec<N> r;
      for (unsigned i = 0; i < N; ++i) r[i] = !in[i];
      HIERARCHY_EXIT();
      return r; 
  }

  template <unsigned N> bvec<N> And(bvec<N> a, bvec<N> b) {
    HIERARCHY_ENTER();
    bvec<N> r(OpElementwise<N, And>(a, b));
    HIERARCHY_EXIT();
    return r;
  }

  template <unsigned N> bvec<N> Or (bvec<N> a, bvec<N> b) {
    HIERARCHY_ENTER();
    bvec<N> r(OpElementwise<N,  Or>(a, b));
    HIERARCHY_EXIT();
    return r;
  }

  template <unsigned N> bvec<N> Xor(bvec<N> a, bvec<N> b) {
    HIERARCHY_ENTER();
    bvec<N> r(OpElementwise<N, Xor>(a, b));
    HIERARCHY_EXIT();
    return r;
  }

  // Those same operations in all-reduce form
  template <unsigned N> node AndN(bvec<N> in) {
    HIERARCHY_ENTER();
    node r(OpReduce<N, And>(in));
    HIERARCHY_EXIT();
    return r;
  }

  template <unsigned N> node OrN (bvec<N> in) {
    HIERARCHY_ENTER();
    node r(OpReduce<N,  Or>(in));
    HIERARCHY_EXIT();
    return r;
  }

  template <unsigned N> node XorN(bvec<N> in) {
    HIERARCHY_ENTER();
    node r(OpReduce<N, Xor>(in));
    HIERARCHY_EXIT();
    return r;
  }

  // Detect whether two values are equal
  template <unsigned N> node EqDetect(bvec<N> a, bvec<N> b) {
    HIERARCHY_ENTER();
    node r(AndN(Not(Xor(a, b))));
    HIERARCHY_EXIT();
    return r;
  }
};

#endif
