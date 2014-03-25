// The assertion system allows certain error conditions to cause simulation to
// finish with an error message
#include <iostream>
#include <cstdlib>

#include "reset.h"
#include "sim.h"
#include "nodeimpl.h"
#include "assert.h"
#include "tickable.h"
#include "tap.h"

using namespace std;
using namespace chdl;

struct assertion_t : public tickable {
  assertion_t(string &s, const node &n): message(s), x(n) { gtap(n); }

  void tick();
  void tock() {}

  node x;
  string message;
};

void assertion_t::tick() {
  if (!nodes[x]->eval()) {
    cerr << "Cycle " << sim_time() << ": " << message << endl;
    abort();
  }
}

static vector<assertion_t *> assertions;

void chdl::assert(std::string text, node n) {
  assertions.push_back(new assertion_t(text, n));
}

void reset_assertions() {
  while (!assertions.empty()) {
    delete *assertions.rbegin();
    assertions.pop_back();
  }
}

CHDL_REGISTER_RESET(reset_assertions);
