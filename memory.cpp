#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <set>

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

struct memory;

// Set of all currently-existing memory objects.
set<memory *> memories;

struct memory : public tickable {
  memory(vector<node> &qa, vector<node> &d, vector<node> &da, node w,
         string filename, bool sync);
  ~memory() { memories.erase(this); }

  void tick();
  void tock();

  void print(ostream &out);

  node w;
  vector<node> qa, da, d, q;

  bool do_write;
  vector<bool> wrdata;

  size_t raddr, waddr;
  vector<bool> contents, rdval;
  string filename;

  bool sync;
};

void memory::tick() {
  do_write = nodes[w]->eval();
  if (sync) raddr = toUint(qa);
  waddr = toUint(da);

  if (do_write)
    for (unsigned i = 0; i < d.size(); ++i)
      wrdata[i] = nodes[d[i]]->eval();
}

void memory::tock() {
  if (sync)
    for (unsigned i = 0; i < d.size(); ++i)
      rdval[i] = contents[raddr*d.size() + i];

  if (do_write)
    for (unsigned i = 0; i < d.size(); ++i)
      contents[waddr*d.size() + i] = wrdata[i];
}

void memory::print(ostream &out) {
  out << "  " << (sync?"sync":"") << "ram <" << qa.size() << ' ' << d.size();
  if (filename != "") out << " \"" << filename << '"';
  out << '>';
  for (unsigned i = 0; i < da.size(); ++i) out << ' ' << da[i];
  for (unsigned i = 0; i < d.size(); ++i) out << ' ' << d[i];
  out << ' ' << w;
  for (unsigned i = 0; i < qa.size(); ++i) out << ' ' << qa[i];
  for (unsigned i = 0; i < q.size(); ++i) out << ' ' << q[i];
  out << endl;
}

struct qnodeimpl : public nodeimpl {
  qnodeimpl(memory *mem, unsigned idx): mem(mem), idx(idx) {
    for (unsigned i = 0; i < mem->qa.size(); ++i) src.push_back(mem->qa[i]);
    for (unsigned i = 0; i < mem->da.size(); ++i) src.push_back(mem->da[i]);
    src.push_back(mem->d[idx]);
    src.push_back(mem->w);
  }

  bool eval() {
    if (mem->sync)
      return mem->rdval[idx];
    //else
      return mem->contents[toUint(mem->qa)*mem->d.size() + idx];
  }

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
      if (i*n + j < contents.size()) contents[i*n + j] = val & 1;
      val >>= 1;
    }
    ++i;
  }
 
  for (size_t j = i*n; j < contents.size(); ++j) contents[j] = i%2;
}

memory::memory(
  vector<node> &qai, vector<node> &di, vector<node> &dai, node w,
  string filename, bool sync
) :
  contents(di.size()<<(qai.size())), wrdata(di.size()-1), filename(filename),
  w(w), qa(qai.size()), d(di.size()), da(qai.size()), raddr(0), waddr(0),
  sync(sync), rdval(di.size())
{
  // Load contents from file
  if (filename != "") load_contents(d.size(), contents, filename);

  // Populate the address arrays.
  for (unsigned i = 0; i < qa.size(); ++i) {
    qa[i] = qai[i];
    da[i] = dai[i];
  }

  // Create the q bits.
  for (unsigned i = 0; i < d.size(); ++i) {
    d[i] = di[i];
    q.push_back((new qnodeimpl(this, i))->id);
  }

  memories.insert(this);
}

// The function used by memory.h, and the simplest of all. Just create a new
// memory with the given inputs and return the outputs.
namespace chdl {
  vector<node> memory_internal(
    vector<node> &qa, vector<node> &d, vector<node> &da, node w,
    string filename, bool sync
  )
  {
    memory *m = new memory(qa, d, da, w, filename, sync);
    return m->q;
  }
};
