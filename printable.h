// Node implementation.
#ifndef CHDL_PRINTABLE_H
#define CHDL_PRINTABLE_H

#include <vector>
#include <set>
#include <iostream>

namespace chdl {
  enum print_lang {
    PRINT_LANG_NETLIST = 0,
    PRINT_LANG_VERILOG = 1
  };

  typedef int print_phase;
  
  struct printable;

  std::set<printable*> &printables();
  
  struct printable {
    printable() { printables().insert(this); }
    ~printable() { printables().erase(this); }

    static void register_print_phase(print_lang l, print_phase p);
    
    virtual bool is_initial(print_lang l, print_phase p) = 0;
    virtual void predecessors(print_lang, print_phase, std::set<printable *> &)
      = 0;
    virtual void print(std::ostream &out, print_lang l, print_phase p) = 0;
  };

  static void get_initial(print_lang l, print_phase p, std::set<printable*> &s);

  void print_design(std::ostream &out, print_lang l);
}

#endif
