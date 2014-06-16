#include "cdomain.h"
#include "tickable.h"

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
  return h;  
}

void chdl::set_clock_domain(cdomain_handle_t cd) {
  cout << "Setting clock domain to " << cd << ", from "
       << cur_clock_domain() << endl;

  cur_clock_domain() = cd;
}

static vector<cdomain_handle_t> &cdomainstack() {
  static vector<cdomain_handle_t> ti{0};
  return ti;
}

cdomain_handle_t chdl::push_clock_domain(unsigned interval) {
  set_clock_domain(new_clock_domain());
  cdomainstack().push_back(cur_clock_domain());
}

cdomain_handle_t chdl::pop_clock_domain() {
  cdomainstack().pop_back();
  cur_clock_domain() = *(cdomainstack().rbegin());
}
