#ifndef __BUS_H
#define __BUS_H

#include "tristate.h"
#include "bvec.h"

namespace chdl {
  template <unsigned N> class bus : public vec<N, tristatenode> {
   public:
    bus() : vec<N, tristatenode>() {}
    bus(const tristatenode &t): vec<N, tristatenode>(t) {}
    
    operator bvec<N>() const {
      bvec<N> r;
      for (unsigned i = 0; i < N; ++i)
        r[i] = vec<N, tristatenode>::nodes[i];
      return r;
    }

    void connect(bvec<N> input, node enable) {
      for (unsigned i = 0; i < N; ++i)
        vec<N, tristatenode>::nodes[i].connect(input[i], enable);
    }
  };
}

#endif
