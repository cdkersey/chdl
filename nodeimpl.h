// Node implementation.
#ifndef CHDL_NODEIMPL_H
#define CHDL_NODEIMPL_H

#include <vector>
#include <iostream>
#include <typeinfo>

#include "node.h"
#include "hierarchy.h"
#include "cdomain.h"
#include "printable.h"

namespace chdl {
  struct nodeimpl;

  extern std::vector<nodeimpl*> nodes;

  struct nodeimpl : public printable {
    nodeimpl() {
      register_print_phase(PRINT_LANG_VERILOG, 10);
      register_print_phase(PRINT_LANG_VERILOG, 100);
      register_print_phase(PRINT_LANG_NETLIST, 100);
      id = nodes.size();
      nodes.push_back(this);
      path = get_hpath();
    }
    virtual ~nodeimpl() {}

    virtual bool eval(cdomain_handle_t) = 0;
    virtual void reset() {}

    virtual void print(std::ostream &) = 0;
    virtual void print_vl(std::ostream &) = 0;

    virtual bool is_initial(print_lang l, print_phase p) {
      return (l == PRINT_LANG_VERILOG) && (p == 10);
    }

    virtual void predecessors(print_lang, print_phase, std::set<printable *> &);
    virtual void print(std::ostream &out, print_lang l, print_phase p);
    
    // The node keeps a copy of its ID. This makes mapping from nodeimpl
    // pointers to node objects straightforward.
    nodeid_t id;

    // Where is this node in the hierarchy?
    hpath_t path;

    std::vector<node> src;
  };
};

#endif
