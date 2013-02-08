// A bvec is a fixed-length vector of bits. rvecs are like bvecs with a connect
// function.
#ifndef __BVEC_BASIC_H
#define __BVEC_BASIC_H

#include "bvec.h"
#include "gates.h"

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
    bvec<A + 1> r;
    r[0] = b;
    for (unsigned i = 0; i < A; ++i) r[i + 1] = a[i];
    return r;
  }

  template <unsigned B> bvec<B + 1> Cat(node a, bvec<B> b) {
    bvec<B + 1> r;
    for (unsigned i = 0; i < B; ++i) r[i] = b[i];
    r[B] = a;
    return r;
  }

  // Create an array of registers.
  template <unsigned N> rvec<N> Reg() {
    rvec<N> r;
    for (unsigned i = 0; i < N; ++i) r[i] = Reg();
    return r;
  }

  template <unsigned N> bvec<N> Reg(bvec<N> d) {
    rvec<N> r;
    for (unsigned i = 0; i < N; ++i) r[i] = Reg(d[i]);
    return r;
  }

  // Add a write signal to an existing array of registers
  template <unsigned N> void Wreg(rvec<N> q, bvec<N> d, node w) {
    for (unsigned i = 0; i < N; ++i)
      q[i].connect(Mux(w, q[i], d[i]));
  }

  // Create an array of registers with a "write" signal
  template <unsigned N> rvec<N> Wreg(node w, bvec<N> d) {
    rvec<N> r(Reg<N>());
    Wreg(r, d, w);
    return r;
  }

  // Create a binary integer literal with the given value
  template <unsigned N> bvec<N> Lit(unsigned long val) {
    bvec<N> r;
    for (size_t i = 0; i < N; ++i) r[i] = Lit((val>>i)&1);
    return r;
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
      bvec<N> r;
      for (unsigned i = 0; i < N; ++i) r[i] = !in[i];
      return r; 
  }

  template <unsigned N>
    bvec<N> And(bvec<N> a, bvec<N> b) { return OpElementwise<N, And>(a, b); }
  template <unsigned N>
    bvec<N> Or (bvec<N> a, bvec<N> b) { return OpElementwise<N,  Or>(a, b); }
  template <unsigned N>
    bvec<N> Xor(bvec<N> a, bvec<N> b) { return OpElementwise<N, Xor>(a, b); }

  // Those same operations in all-reduce form
  template <unsigned N>
    node AndN(bvec<N> in) { return OpReduce<N, And>(in); }
  template <unsigned N>
    node OrN (bvec<N> in) { return OpReduce<N,  Or>(in); }
  template <unsigned N>
    node XorN(bvec<N> in) { return OpReduce<N, Xor>(in); }

  // Detect whether two values are equal
  template <unsigned N> node EqDetect(bvec<N> a, bvec<N> b) {
    return AndN(Not(Xor(a, b)));
  }
};

#endif
