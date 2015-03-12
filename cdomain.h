// Clock domain support.
#ifndef CHDL_CDOMAIN_H
#define CHDL_CDOMAIN_H

#include <vector>

namespace chdl {
  typedef unsigned cdomain_handle_t;

  std::vector<unsigned> &tick_intervals();
  cdomain_handle_t &cur_clock_domain();

  cdomain_handle_t new_clock_domain(unsigned interval=1);
  void set_clock_domain(cdomain_handle_t cd);
  cdomain_handle_t push_clock_domain(unsigned interval=1);
  cdomain_handle_t pop_clock_domain();
  unsigned clock_domains(); // Return number of clock domains
};

#endif
