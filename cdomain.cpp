#include "cdomain.h"
#include "tickable.h"
#include "sim.h"

#include <vector>
#include <stack>
#include <iostream>

using namespace chdl;
using namespace std;

cdomain_handle_t &chdl::cur_clock_domain() {
  static cdomain_handle_t cd(0);
  return cd;
}

vector<unsigned> &chdl::tick_intervals() {
  static vector<unsigned> ti{1};
  return ti;
}

cdomain_handle_t chdl::new_clock_domain(unsigned interval) {
  cdomain_handle_t h(tick_intervals().size());
  tick_intervals().push_back(interval);
  tickables().push_back(vector<tickable*>());
  now.push_back(0);
  return h;  
}

void chdl::set_clock_domain(cdomain_handle_t cd) {
  cur_clock_domain() = cd;
}

static vector<cdomain_handle_t> &cdomainstack() {
  static vector<cdomain_handle_t> s{0};
  return s;
}

cdomain_handle_t chdl::push_clock_domain(unsigned interval) {
  set_clock_domain(new_clock_domain(interval));
  cdomainstack().push_back(cur_clock_domain());
  return cur_clock_domain();
}

cdomain_handle_t chdl::pop_clock_domain() {
  cdomainstack().pop_back();
  cur_clock_domain() = *(cdomainstack().rbegin());
  return cur_clock_domain();
}

unsigned chdl::clock_domains() { return tickables().size(); }
