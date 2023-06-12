#ifndef CHDL_INPUTIMPL_H
#define CHDL_INPUTIMPL_H

#include <map>
#include <string>

#include "input.h"
#include "nodeimpl.h"

namespace chdl {
  class inputimpl : public nodeimpl {
    public:
      inputimpl(std::string n, int i=-1): nodeimpl(), name(n), pos(i) {}

      bool eval(cdomain_handle_t) { return 0; }
      bool is_initial(print_lang l, print_phase p) { return (p == 100); }

      void print(std::ostream &) {}
      void print_vl(std::ostream &);

    private:
      std::string name; // Name of input in input map
      int pos;           // Position in vector in input map. -1 for no name
  };
}

#endif
