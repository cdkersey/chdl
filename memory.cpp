#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <set>
#include <random>
#include <cstdint>

#include "memory.h"
#include "tickable.h"
#include "nodeimpl.h"
#include "regimpl.h"
#include "analysis.h"
#include "reset.h"
#include "cdomain.h"

using namespace chdl;
using namespace std;

unsigned long toUint(vector<node> &v, cdomain_handle_t cd) {
  unsigned long r(0);
  for (unsigned i = 0; i < v.size(); ++i)
    if (nodes[v[i]]->eval(cd)) r |= 1ul<<i;
  return r;
}

// Set of all currently-existing memory objects.
vector<memory *> memories;

void memory::tick(cdomain_handle_t cd) {
  do_write = nodes[w]->eval(cd);
  if (sync)
    for (unsigned i = 0; i < qa.size(); ++i) raddr[i] = toUint(qa[i], cd);

  waddr = toUint(da, cd);

  if (do_write)
    for (unsigned i = 0; i < d.size(); ++i)
      wrdata[i] = nodes[d[i]]->eval(cd);
}

void memory::tock(cdomain_handle_t) {
  if (sync)
    for (unsigned i = 0; i < q.size(); ++i)
      for (unsigned j = 0; j < d.size(); ++j)
        rdval[i][j] = contents[raddr[i]*d.size() + j];

  if (do_write)
    for (unsigned i = 0; i < d.size(); ++i)
      contents[waddr*d.size() + i] = wrdata[i];
}

static bool exists(nodeid_t x) { return x != NO_NODE; }

void memory::print(ostream &out) {
  out << "  " << (sync?"sync":"") << "ram <" << qa[0].size() << ' ' << d.size();
  if (filename != "") out << " \"" << filename << '"';
  out << '>';
  for (unsigned i = 0; i < da.size(); ++i) out << ' ' << da[i];
  for (unsigned i = 0; i < d.size(); ++i) out << ' ' << d[i];
  out << ' ' << w;
  for (unsigned j = 0; j < qa.size(); ++j) {
    for (unsigned i = 0; i < qa[0].size(); ++i) out << ' ' << qa[j][i];
    for (unsigned i = 0; i < q[0].size(); ++i)
      if (exists(q[j][i])) out << ' ' << q[j][i];
  }
  out << endl;
}

void memory::print_vl(ostream &out) {
  // Some FPGA toolchains need a synchronous read. This lets us run our designs
  // on these FPGAs, albeit with degraded performance.
  const bool PSEUDO_ASYNC = true && !sync;
  
  set<unsigned> dead_ports; // HACK
  unsigned id(q[0][0]);

  // HACK: identify dead ports
  for (unsigned i = 0; i < q.size(); ++i) {
    bool dead = true;
    for (unsigned j = 0; j < q[i].size(); ++j)
      if (q[i][j] != NO_NODE) dead = false;
    if (dead) dead_ports.insert(i);
  }

  size_t words(1ul<<da.size()), bits(d.size());
  for (unsigned i = 0; i < qa.size(); ++i) {
    if (dead_ports.count(i)) continue;
    out << "  wire [" << qa[0].size()-1 << ":0] __mem_qa" << id << '_' << i
        << ';' << endl
        << "  " << ((sync || PSEUDO_ASYNC)?"reg":"wire")
        << " [" << bits-1 << ":0] __mem_q" << id << '_' << i <<  ';'
        << endl;
  }

  out << "  wire [" << da.size()-1 << ":0] __mem_da" << id << ';' << endl
      << "  wire [" << bits-1 << ":0] __mem_d" << id << ';' << endl
      << "  wire __mem_w" << id << ';' << endl
      << "  reg [" << bits-1 << ":0] __mem_array" << id
      << '[' << words-1 << ":0];" << endl;

  if (!sync && !PSEUDO_ASYNC) {
    for (unsigned i = 0; i < q.size(); ++i) {
      out << "  assign __mem_q" << id << '_' << i << " = __mem_array"
          << id << "[__mem_qa" << id << '_' << i << "];" << endl;
    }
  }

  out << "  always @(" << (!sync && PSEUDO_ASYNC ? "neg" : "pos")
      << "edge phi";
  if (cd != 0) out << cd;
  out << ")" << endl << "    begin" << endl;

  if (sync || PSEUDO_ASYNC) {
    for (unsigned i = 0; i < q.size(); ++i) {
      if (dead_ports.count(i)) continue;
      out << "      __mem_q" << id << '_' << i
          << " <= __mem_array" << id << "[__mem_qa" << id << '_' << i << "];"
          << endl;
    }
  }

  out << "      if (__mem_w" << id << ") __mem_array" << id
      << "[__mem_da" << id << "] <= __mem_d" << id << ';' << endl
      << "  end" << endl;

  out << "  assign __mem_w" << id << " = __x" << w << ';' << endl;
  for (unsigned j = 0; j < qa.size(); ++j) {
    if (dead_ports.count(j)) continue;
    for (unsigned i = 0; i < qa[0].size(); ++i)
      out << "  assign __mem_qa" << id << '_' << j << '[' << i << "] = __x"
          << qa[j][i] << ';' << endl;
  }

  for (unsigned i = 0; i < da.size(); ++i) {
    out << "  assign __mem_da" << id << '[' << i << "] = __x" << da[i] << ';'
        << endl;
  }

  for (unsigned j = 0; j < q.size(); ++j) {
    if (dead_ports.count(j)) continue;
    for (unsigned i = 0; i < q[j].size(); ++i) {
      out << "  assign __x" << q[j][i] << " = __mem_q" << id << '_' << j
          << '[' << i << "];" << endl;
    }
  }
  for (unsigned i = 0; i < d.size(); ++i)
    out << "  assign __mem_d" << id << '[' << i << "] = __x" << d[i] << ';'
        << endl;
}

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
  else {
    // Default SRAM contents: random bits; seed with pointer to take advantage
    // of ASR.
    minstd_rand m0((unsigned)reinterpret_cast<intptr_t>(this));
    for (unsigned i = 0; i < contents.size(); ++i)
      contents[i] = (m0() & 1);
  }

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

bool chdl::qnodeimpl::eval(cdomain_handle_t cd) {
  bool new_cval;

  if (t_cval != sim_time(cd)) {
    t_cval = sim_time(cd);

    if (mem->sync)
      new_cval = mem->rdval[port][idx];
    else
      new_cval = mem->contents[toUint(mem->qa[port], cd)*mem->d.size() + idx];

    cval = new_cval;
  }

  return cval;
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

void chdl::get_mem_q_nodes(set<nodeid_t> &s) {
  for (nodeid_t n = 0; n < nodes.size(); ++n)
    if (dynamic_cast<qnodeimpl*>(nodes[n]))
      s.insert(n);
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

void chdl::get_mem_params(memory *&ptr, unsigned &bit,
                          unsigned &port, unsigned &ports,
                          unsigned &a, unsigned &d, nodeid_t n)
{
  qnodeimpl* p(dynamic_cast<qnodeimpl*>(nodes[n]));
  if (!p) {
    ptr = NULL;
  } else {
    bit = p->idx;
    port = p->port;
    ptr = p->mem;
    ports = ptr->rdval.size();
    a = ptr->qa[0].size();
    d = ptr->d.size();
  }
}

CHDL_REGISTER_RESET(reset_memories);
