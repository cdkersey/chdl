// Node implementation.
#ifndef __NODEIMPL_H
#define __NODEIMPL_H

#include <vector>
#include <iostream>
#include <functional>

#include "node.h"
#include "hierarchy.h"
#include "cdomain.h"
#include "execbuf.h"

namespace chdl {
  struct nodeimpl;

  extern std::vector<nodeimpl*> nodes;

  typedef std::vector<unsigned> nodebuf_t;
  typedef std::function<bool(nodeid_t)> evaluator_t;

  struct nodeimpl {
    nodeimpl() { id = nodes.size(); nodes.push_back(this); path = get_hpath(); }
    virtual ~nodeimpl() {}

    virtual bool eval(evaluator_t &e) = 0;
    virtual void print(std::ostream &) = 0;
    virtual void print_vl(std::ostream &) = 0;

    virtual void gen_eval(evaluator_t &e, execbuf &b, nodebuf_t &from);
    virtual void gen_store_result(execbuf &b, nodebuf_t &from, nodebuf_t &to);

    // The node keeps a copy of its ID. This makes mapping from nodeimpl
    // pointers to node objects straightforward.
    nodeid_t id;

    // Where is this node in the hierarchy?
    hpath_t path;

    std::vector<node> src;
  };
};

#endif
