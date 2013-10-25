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
#include "analysis.h"
#include "reset.h"

using namespace chdl;
using namespace std;

unsigned long toUint(vector<node> &v) {
  unsigned long r(0);
  for (unsigned i = 0; i < v.size(); ++i)
    if (nodes[v[i]]->eval()) r |= 1ul<<i;
  return r;
}

struct memory;

// Set of all currently-existing memory objects.
vector<memory *> memories;

struct memory : public tickable {
  memory(vector<node> &qa, vector<node> &d, vector<node> &da, node w,
         string filename, bool sync, size_t &id);
  ~memory() { cout << "Destructing a memory." << endl; }

  vector<node> add_read_port(vector<node> &qa);

  void tick();
  void tock();

  void print(ostream &out);
  void print_vl(ostream &out);

  node w;
  vector<node> da, d;
  vector<vector<node>> qa, q;

  bool do_write;
  vector<bool> wrdata;

  size_t waddr;
  vector<size_t> raddr;
  vector<bool> contents;
  vector<vector<bool>> rdval;
  string filename;

  bool sync;
};

void memory::tick() {
  do_write = nodes[w]->eval();
  if (sync) for (unsigned i = 0; i < qa.size(); ++i) raddr[i] = toUint(qa[i]);
  waddr = toUint(da);

  if (do_write)
    for (unsigned i = 0; i < d.size(); ++i)
      wrdata[i] = nodes[d[i]]->eval();
}

void memory::tock() {
  if (sync)
    for (unsigned i = 0; i < q.size(); ++i)
      for (unsigned j = 0; j < d.size(); ++j)
        rdval[i][j] = contents[raddr[i]*d.size() + j];

  if (do_write)
    for (unsigned i = 0; i < d.size(); ++i)
      contents[waddr*d.size() + i] = wrdata[i];
}

void memory::print(ostream &out) {
  out << "  " << (sync?"sync":"") << "ram <" << qa[0].size() << ' ' << d.size();
  if (filename != "") out << " \"" << filename << '"';
  out << '>';
  for (unsigned i = 0; i < da.size(); ++i) out << ' ' << da[i];
  for (unsigned i = 0; i < d.size(); ++i) out << ' ' << d[i];
  out << ' ' << w;
  for (unsigned j = 0; j < qa.size(); ++j) {
    for (unsigned i = 0; i < qa[0].size(); ++i) out << ' ' << qa[j][i];
    for (unsigned i = 0; i < q[0].size(); ++i) out << ' ' << q[j][i];
  }
  out << endl;
}

void memory::print_vl(ostream &out) {
  unsigned id(q[0][0]);

  if (!sync) {
    cerr << "Async RAM not currently supported in verilog. Use llmem." << endl;
    exit(1);
  }

  size_t words(1ul<<da.size()), bits(d.size());
  for (unsigned i = 0; i < qa.size(); ++i) {
    out << "  wire [" << qa[0].size()-1 << ":0] __mem_qa" << id << '_' << i
        << ';' << endl
        << "  reg [" << bits-1 << ":0] __mem_q" << id << '_' << i <<  ';'
        << endl;
  }

  out << "  wire [" << da.size()-1 << ":0] __mem_da" << id << ';' << endl
      << "  wire [" << bits-1 << ":0] __mem_d" << id << ';' << endl
      << "  wire __mem_w" << id << ';' << endl
      << "  reg [" << bits-1 << ":0] __mem_array" << id
      << '[' << words-1 << ":0];" << endl
      << "  always @(posedge phi)" << endl
      << "    begin" << endl;
  for (unsigned i = 0; i < q.size(); ++i) {
    out << "      __mem_q" << id << '_' << i
        << " <= __mem_array" << id << "[__mem_qa" << id << '_' << i << "];"
        << endl;
  }

  out << "      if (__mem_w" << id << ") __mem_array" << id
      << "[__mem_da" << id << "] <= __mem_d" << id << ';' << endl
      << "  end" << endl;
  
  out << "  assign __mem_w" << id << " = __x" << w << ';' << endl;
  for (unsigned j = 0; j < qa.size(); ++j) {
    for (unsigned i = 0; i < qa[0].size(); ++i)
      out << "  assign __mem_qa" << id << '_' << j << '[' << i << "] = __x"
          << qa[j][i] << ';' << endl;
  }

  for (unsigned i = 0; i < da.size(); ++i)
    out << "  assign __mem_da" << id << '[' << i << "] = __x" << da[i] << ';'
        << endl;
  for (unsigned j = 0; j < q.size(); ++j) {
    for (unsigned i = 0; i < q[j].size(); ++i) {
      out << "  assign __x" << q[j][i] << " = __mem_q" << id << '_' << j
          << '[' << i << "];" << endl;
    }
  }
  for (unsigned i = 0; i < d.size(); ++i)
    out << "  assign __mem_d" << id << '[' << i << "] = __x" << d[i] << ';'
        << endl;
}

struct qnodeimpl : public nodeimpl {
  qnodeimpl(memory *mem, unsigned port, unsigned idx):
    mem(mem), port(port), idx(idx) {}

  bool eval() {
    if (mem->sync)
      return mem->rdval[port][idx];
    else
      return mem->contents[toUint(mem->qa[port])*mem->d.size() + idx];
  }

  void print(ostream &out) { if (port == 0 && idx == 0) mem->print(out); }
  void print_vl(ostream &out) { if (port == 0 && idx == 0) mem->print_vl(out); }

  unsigned port, idx;

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
  string filename, bool sync, size_t &id
) :
  contents(di.size()<<(dai.size())), wrdata(di.size()), filename(filename),
  w(w), raddr(0), waddr(0), da(dai), d(di), sync(sync)
{
  // Load contents from file
  if (filename != "") load_contents(di.size(), contents, filename);

  // Add the read port
  add_read_port(qai);

  id = memories.size();
  memories.push_back(this);
}

// The function used by memory.h, and the simplest of all. Just create a new
// memory with the given inputs and return the outputs.
namespace chdl {
  vector<node> memory_internal(
    vector<node> &qa, vector<node> &d, vector<node> &da, node w,
    string filename, bool sync, size_t &id
  )
  {
    memory *m = new memory(qa, d, da, w, filename, sync, id);
    return m->q[0];
  }

  vector<node> memory_add_read_port(size_t id, vector<node> &qa) {
    return memories[id]->add_read_port(qa);
  }
};

vector<node> memory::add_read_port(vector<node> &qai) {
  size_t idx(q.size());

  qa.push_back(qai);
  raddr.push_back(0);
  rdval.push_back(vector<bool>(d.size()));

  q.push_back(vector<node>());
  for (unsigned i = 0; i < d.size(); ++i)
    q[idx].push_back((new qnodeimpl(this, idx, i))->id);

  return q[idx];
}

void chdl::get_mem_nodes(set<nodeid_t> &s) {
  for (auto mp : memories) {
    memory &m(*mp);
    for (unsigned j = 0; j < m.qa.size(); ++j)
      for (unsigned i = 0; i < m.qa[j].size(); ++i) s.insert(m.qa[j][i]);
    for (unsigned i = 0; i < m.da.size(); ++i) s.insert(m.da[i]);
    for (unsigned i = 0; i < m.d.size(); ++i)  s.insert(m.d[i]);
    s.insert(m.w);
  }
}

size_t chdl::num_sram_bits() {
  size_t count(0);
  for (auto m : memories)
    count += m->contents.size();
  return count;
}

static void reset_memories() {
  for (auto m : memories) delete m;
  memories.clear();
}

CHDL_REGISTER_RESET(reset_memories);
