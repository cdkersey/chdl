#include "reset.h"

using namespace chdl;
using namespace std;

reset_func_base::~reset_func_base() {}

vector<reset_func_base*> *chdl::reset_func_base::rfuns() {
  static vector<reset_func_base*> *rf(new vector<reset_func_base*>());
  return rf;
}

void chdl::reset() { for (auto f : *reset_func_base::rfuns()) (*f)(); }
