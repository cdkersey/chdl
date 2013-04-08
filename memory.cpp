#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <set>

#include "memory.h"
#include "tickable.h"
#include "nodeimpl.h"
#include "regimpl.h"

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
  void print_vl(ostream &out);

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

void memory::print_vl(ostream &out) {
  unsigned id(d[0]);

  if (!sync) {
    cerr << "Async RAM not currently supported in verilog. Use llmem." << endl;
    exit(1);
  }

  size_t words(1ul<<qa.size()), bits(d.size());
  out << "  wire [" << qa.size()-1 << ":0] __mem_qa" << id << ';' << endl
      << "  reg [" << bits-1 << ":0] __mem_q" << id << ';' << endl
      << "  wire [" << da.size()-1 << ":0] __mem_da" << id << ';' << endl
      << "  wire [" << bits-1 << ":0] __mem_d" << id << ';' << endl
      << "  wire __mem_w" << id << ';' << endl
      << "  reg [" << bits-1 << ":0] __mem_array" << id
      << '[' << words-1 << ":0];" << endl
      << "  always @(posedge phi)" << endl
      << "    begin" << endl
      << "      __mem_q" << id
      << " <= __mem_array" << id << "[__mem_da" << id << "];" << endl
      << "      if (__mem_w" << id << ") __mem_array" << id
      << "[__mem_da" << id << "] <= __mem_d" << id << ';' << endl
      << "  end" << endl;
  
  out << "  assign __mem_w" << id << " = __x" << w << ';' << endl;
  for (unsigned i = 0; i < qa.size(); ++i)
    out << "  assign __mem_qa" << id << '[' << i << "] = __x" << qa[i] << ';'
        << endl;
  for (unsigned i = 0; i < da.size(); ++i)
    out << "  assign __mem_da" << id << '[' << i << "] = __x" << da[i] << ';'
        << endl;
  for (unsigned i = 0; i < q.size(); ++i)
    out << "  assign __x" << q[i] << " = __mem_q" << id << '[' << i << "];"
        << endl;
  for (unsigned i = 0; i < d.size(); ++i)
    out << "  assign __mem_d" << id << '[' << i << "] = __x" << d[i] << ';'
        << endl;
}

struct qnodeimpl : public nodeimpl {
  qnodeimpl(memory *mem, unsigned idx): mem(mem), idx(idx) {}

  bool eval() {
    if (mem->sync)
      return mem->rdval[idx];
    else
      return mem->contents[toUint(mem->qa)*mem->d.size() + idx];
  }

  void print(ostream &out) { if (idx == 0) mem->print(out); }
  void print_vl(ostream &out) { if (idx == 0) mem->print_vl(out); }

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

void chdl::get_mem_nodes(set<nodeid_t> &s) {
  for (auto it = memories.begin(); it != memories.end(); ++it) {
    memory &m(**it);
    for (unsigned i = 0; i < m.qa.size(); ++i) s.insert(m.qa[i]);
    for (unsigned i = 0; i < m.da.size(); ++i) s.insert(m.da[i]);
    for (unsigned i = 0; i < m.d.size(); ++i)  s.insert(m.d[i]);
    s.insert(m.w);
  }
}
