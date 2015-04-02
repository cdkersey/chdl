// The basic node type is merely an array index to the master array of nodeimpl
// pointers. This makes it easy to copy about and also to change the underlying
// implementation. Any optimizations invalidate all still-in-use node objects,
// and must only be called after the design has been fully entered.
#ifndef __NODE_H
#define __NODE_H

#include <map>
#include <set>

#include <stdlib.h>

namespace chdl {
  typedef unsigned long nodeid_t;

  const nodeid_t NO_NODE(~nodeid_t(0));

  nodeid_t nodecount();

  class node {
  public:
    // Sets the default index to an improbable value so attempts to use this as
    // a valid node will fail.
    node();
    node(nodeid_t i);
    node(const node &r);
 
    virtual ~node();

    operator nodeid_t() const { return idx; }

    // Our weird overloaded assignment operator can actually be const, since we
    // don't directly change the idx of this object through the this pointer.
    const node &operator=(const node &r) const;
    node &operator=(const node &r);

    void change_net(nodeid_t i);

    void check() const { if (idx != NO_NODE && idx > nodecount()) abort(); }

  protected:
    nodeid_t idx;
  };

  // Get all of the nodeids of nodeimpls to which no node points.
  void get_dead_nodes(std::set<nodeid_t> &s);

  // Given a map from old node id to new node id, rearrange all of the nodes to
  // fit. 
  void permute_nodes(std::map<nodeid_t, nodeid_t> x);
};

#endif
