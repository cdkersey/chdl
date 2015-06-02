// Tickable.
#include "tickable.h"
#include "cdomain.h"
#include "reset.h"

using namespace std;
using namespace chdl;

vector<vector<tickable*> > &chdl::tickables() {
  static vector<vector<tickable*> > t(1);
  return t;
}

static void reset_tickables() {
  tickables() = vector<vector<tickable*> >(1);
}

CHDL_REGISTER_RESET(reset_tickables);

// Destroying a tickable is O(N^2)
tickable::~tickable() {
  vector<tickable*> dest;
  for (unsigned j = 0; j < tickables().size(); ++j) {
    bool found(false);
    for (unsigned i = 0; i < tickables()[j].size(); ++i)
      if (tickables()[j][i] == this) found = true;

    if (found) {
      for (unsigned i = 0; i < tickables()[j].size(); ++i)
        if (tickables()[j][i] != this) dest.push_back(tickables()[j][i]);

      tickables()[j] = dest;
    }
  }
}
