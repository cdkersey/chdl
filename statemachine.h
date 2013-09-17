#ifndef __STATEMACHINE_H
#define __STATEMACHINE_H

#include "bvec-basic-op.h"
#include "adder.h"
#include "shifter.h"
#include "enc.h"
#include "mux.h"
#include "tap.h"

namespace chdl {
  // A state machine with N states.
  template <unsigned N> class Statemachine {
   public:
  Statemachine(): nextState(Enc(nextState1h)), state(Reg(nextState)) {
      state1h = Zext<N>(Decoder(state));
    }

    void edge(unsigned from, unsigned to, node take) {
      edges[to][from] = take && state1h[from];
    }

    void generate() {
      for (unsigned i = 0; i < N; ++i)
        nextState1h[i] = OrN(edges[i]);
    }

    operator bvec<CLOG2(N)>() { return state; }
    bvec<CLOG2(N)> getState() { return state; }
    node inState(unsigned s) { return state1h[s]; }

   private:
    vec<N, bvec<N>> edges;
    bvec<N> nextState1h, state1h;
    bvec<CLOG2(N)> nextState, state;
  };
};

#endif
