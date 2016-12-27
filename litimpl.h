#ifndef __LITIMPL_H
#define __LITIMPL_H

#include "nodeimpl.h"

namespace chdl {
  class litimpl : public nodeimpl {
  public:
    litimpl(bool v): val(v), undef(0) {}
    litimpl(): val(0), undef(1) {}

    virtual bool eval(cdomain_handle_t cd);
    virtual void print(std::ostream &out);
    virtual void print_vl(std::ostream &out);

    virtual bool is_initial(print_lang l, print_phase p) { return true; }
    virtual void print(std::ostream &out, print_lang l, print_phase p);
    
    bool is_undef() { return undef; }
    void resolve(bool x) { if (undef) val = x; else abort(); }

  private:
    bool val, undef;
  };
};

#endif
