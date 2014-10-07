#include "reset.h"

#include <iostream>
#include <set>

using namespace chdl;
using namespace std;

reset_func_base::~reset_func_base() {}

vector<reset_func_base*> *chdl::reset_func_base::rfuns() {
  static vector<reset_func_base*> *rf(new vector<reset_func_base*>());
  return rf;
}

void chdl::reset() { for (auto f : *reset_func_base::rfuns()) (*f)(); }

set<delete_on_reset*> chdl::delete_on_reset::dors;

chdl::delete_on_reset::delete_on_reset() { dors.insert(this); }
chdl::delete_on_reset::~delete_on_reset() { dors.erase(this); }

static void reset_delete_on_reset() {
  while(!chdl::delete_on_reset::dors.empty())
    delete *chdl::delete_on_reset::dors.begin();
}

CHDL_REGISTER_RESET(reset_delete_on_reset);
