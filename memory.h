#ifndef __MEMORY_H
#define __MEMORY_H

#include <string>
#include <vector>
#include <set>

#include "bvec-basic.h"
#include "node.h"
#include "lit.h"
#include "tickable.h"
#include "nodeimpl.h"

#include "hierarchy.h"

namespace chdl {
  void get_mem_nodes(std::set<nodeid_t> &s);
  void get_mem_q_nodes(std::set<nodeid_t> &s);

  struct memory : public tickable {
    memory(std::vector<node> &qa, std::vector<node> &d, std::vector<node> &da,
           node w, std::string filename, bool sync, size_t &id);
    ~memory() {}

    std::vector<node> add_read_port(std::vector<node> &qa);

    void tick(cdomain_handle_t cd);
    void tock(cdomain_handle_t cd);

    void print(std::ostream &out);
    void print_vl(std::ostream &out);

    node w;
    std::vector<node> da, d;
    std::vector<std::vector<node>> qa, q;

    bool do_write;
    std::vector<bool> wrdata;

    size_t waddr;
    std::vector<size_t> raddr;
    std::vector<bool> contents;
    std::vector<std::vector<bool>> rdval;
    std::string filename;

    bool sync;
  };

  struct qnodeimpl : public nodeimpl {
    qnodeimpl(memory *mem, unsigned port, unsigned idx):
      mem(mem), port(port), idx(idx) {}

    bool eval(cdomain_handle_t cd);

    void print(std::ostream &out)
      { if (port == 0 && idx == 0) mem->print(out); }
    void print_vl(std::ostream &out)
      { if (port == 0 && idx == 0) mem->print_vl(out); }

    unsigned port, idx;

    memory *mem;
  };

  // Get pointer and dimensions for memory with a given Q-node
  void get_mem_params(memory *&ptr, unsigned &bit,
                      unsigned &port, unsigned &ports,
                      unsigned &a, unsigned &d, nodeid_t n);
  
  template <unsigned M, unsigned N, unsigned P>
    vec<P, bvec<N>> Memory(
      vec<P, bvec<M>> qa, bvec<N> d, bvec<M> da, node w,
      std::string filename = "", bool sync=false
    )
  {
    HIERARCHY_ENTER();

    // Provides interface to low-level node structures; prototyped here to avoid
    // pollution of chdl namespace.
    std::vector<node> memory_internal(
      std::vector<node> &qa, std::vector<node> &d, std::vector<node> &da,
      node w, std::string filename, bool sync, size_t &id
    );

    std::vector<node> memory_add_read_port(size_t id, std::vector<node> &qa);

    std::vector<std::vector<node>> qavec(P), qvec(P);

    std::vector<node> dvec, davec;
    for (unsigned i = 0; i < M; ++i) {
      for (unsigned j = 0; j < P; ++j)
        qavec[j].push_back(qa[j][i]);
      davec.push_back(da[i]);
    }
    for (unsigned i = 0; i < N; ++i)
      dvec.push_back(d[i]);
  
    size_t id;
    qvec[0] = memory_internal(qavec[0], dvec, davec, w, filename, sync, id);
    for (unsigned i = 1; i < P; ++i)
      qvec[i] = memory_add_read_port(id, qavec[i]);

    vec<P, bvec<N>> q;
    for (unsigned i = 0; i < N; ++i)
      for (unsigned j = 0; j < P; ++j)
        q[j][i] = qvec[j][i];
    
    HIERARCHY_EXIT();

    return q;
  }

  template <unsigned M, unsigned N>
    bvec<N> Memory(
      bvec<M> qa, bvec<N> d, bvec<M> da, node w,
      std::string filename = "", bool sync=false
    )
  {
    return Memory(vec<1, bvec<M>>(qa), d, da, w, filename, sync)[0];
  }

  template <unsigned M, unsigned N>
    bvec<N> Memory(
      bvec<M> a, bvec<N> d, node w, std::string filename = "", bool sync = false
    )
  {
    return Memory(a, d, a, w, filename, sync);
  }

  template <unsigned M, unsigned N, unsigned P>
    vec<P, bvec<N>> Syncmem(
      vec<P, bvec<M>> qa, bvec <N> d, bvec<M> da, node w,
      std::string filename = ""
    )
  {
    return Memory(qa, d, da, w, filename, true);
  }

  template <unsigned M, unsigned N>
    bvec<N> Syncmem(
      bvec<M> qa, bvec <N> d, bvec<M> da, node w, std::string filename = ""
    )
  {
    return Memory(qa, d, da, w, filename, true);
  }

  template <unsigned M, unsigned N>
    bvec<N> Syncmem(
      bvec<M> a, bvec<N> d, node w, std::string filename = ""
    )
  {
    return Syncmem(a, d, a, w, filename);
  }

  template <unsigned M, unsigned N>
    bvec<N> Rom(bvec<M> qa, std::string filename)
  {
    return Memory(qa, Lit<N>(0), Lit<M>(0), Lit(0), filename);
  }

  template <unsigned M, unsigned N>
    bvec<N> Syncrom(bvec<M> qa, std::string filename)
  {
    return Syncmem(qa, Lit<N>(0), Lit<M>(0), Lit(0), filename);
  }
};

#endif
