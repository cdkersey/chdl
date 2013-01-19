#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>

#include "memory.h"
#include "tickable.h"
#include "nodeimpl.h"

using namespace chdl;
using namespace std;

unsigned long toUint(vector<node> &v) {
  unsigned long r(0);
  for (unsigned i = 0; i < v.size(); ++i)
    if (nodes[v[i]]->eval()) r |= 1<<i;
  return r;
}

struct memory : public tickable {
  memory(vector<node> &d, vector<node> &a, node w, string filename);

  void tick();
  void tock();

  void print(ostream &out);

  node w;
  vector<node> a, d, q;

  bool do_write;
  vector<bool> wrdata;

  size_t addr;
  vector<bool> contents;
  string filename;
};

void memory::tick() {
  do_write = nodes[w]->eval();
  addr = toUint(a);

  if (do_write)
    for (unsigned i = 0; i < d.size(); ++i)
      wrdata[i] = nodes[d[i]]->eval();
}

void memory::tock() {
  if (do_write)
    for (unsigned i = 0; i < d.size(); ++i)
      contents[addr*d.size() + i] = wrdata[i];
}

void memory::print(ostream &out) {
  out << "  ram <" << a.size() << ' ' << d.size();
  if (filename != "") out << " \"" << filename << '"';
  out << '>';
  for (unsigned i = 0; i < a.size(); ++i) out << ' ' << a[i];
  for (unsigned i = 0; i < d.size(); ++i) out << ' ' << d[i];
  out << ' ' << w;
  for (unsigned i = 0; i < d.size(); ++i) out << ' ' << q[i];
  out << endl;
}

struct qnodeimpl : public nodeimpl {
  qnodeimpl(memory *mem, unsigned idx): mem(mem), idx(idx) {
    for (unsigned i = 0; i < mem->a.size(); ++i) src.push_back(mem->a[i]);
    src.push_back(mem->d[idx]);
    src.push_back(mem->w);
  }

  bool eval() { return mem->contents[toUint(mem->a)*mem->d.size() + idx]; }
  void print(ostream &out) { if (idx == 0) mem->print(out); }

  unsigned idx;
  
  memory *mem;
};

// Load a hex file.
void load_contents(unsigned n, vector<bool> &contents, string filename) {
  ifstream in(filename.c_str());
  size_t i = 0;
  while (in) {
    unsigned long long val;
    in >> hex >> val;
    for (unsigned j = 0; j < n; ++j) {
      contents[i*n + j] = val & 1;
      val >>= 1;
    }
    ++i;
  }
 
  for (size_t j = i*n; j < contents.size(); ++j) contents[j] = i%2;
}

memory::memory(vector<node> &di, vector<node> &ai, node w, string filename) :
  contents(di.size()<<(ai.size())), wrdata(di.size()-1), filename(filename),
  w(w), d(di.size()), a(ai.size())
{
  // Load contents from file
  if (filename != "") load_contents(d.size(), contents, filename);

  // Populate the address array.
  for (unsigned i = 0; i < a.size(); ++i) a[i] = ai[i];

  // Create the q bits.
  for (unsigned i = 0; i < d.size(); ++i) {
    d[i] = di[i];
    q.push_back((new qnodeimpl(this, i))->id);
  }
}

// The function used by memory.h, and the simplest of all. Just create a new
// memory with the given inputs and return the outputs.
namespace chdl {
  vector<node> memory_internal(
    vector<node> &d, vector<node> &a, node w, string filename
  )
  {
    memory *m = new memory(d, a, w, filename);
    return m->q;
  }
};
