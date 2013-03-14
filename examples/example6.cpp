#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

#include <gateops.h>
#include <bvec-basic-op.h>
#include <adder.h>
#include <shifter.h>
#include <mux.h>
#include <enc.h>
#include <llmem.h>
#include <memory.h>

#include <opt.h>
#include <tap.h>
#include <sim.h>
#include <netlist.h>
#include <analysis.h>
#include <vis.h>

#include <hierarchy.h>

using namespace std;
using namespace chdl;

// TODO:
//   - Add a way to respect hazard destination valid bit (to avoid unnecessary
//     forwarding/stalling)

// Neat little hack that finds the log2 of a power of 2 at compile time,
// suitable for use as a template parameter

// Integer logarithm unit (find index of highest set bit); priority encoder

// Create the OrN tree recursively.
template <typename FUNC> vector<node> ReduceInternal(vector<node> &v, FUNC f) {
  if (v.size() == 1) return v;

  vector<node> v2;
  for (unsigned i = 0; i < v.size(); i += 2) {
    if (i+1 == v.size()) v2.push_back(v[i]);
    else                 v2.push_back(f(v[i], v[i+1]));
  }

  return ReduceInternal<FUNC>(v2, f);
}
// Overload this function for variable-sized vector<node>s instead of bvecs.
node OrN(vector<node> &v) {
  HIERARCHY_ENTER();
  node r;
  if (v.size() == 0) r = Lit(0);
  else r = ReduceInternal<node (*)(node, node)>(v, Or)[0];
  HIERARCHY_EXIT();
  return r;
}

node AndN(vector<node> &v) {
  HIERARCHY_ENTER();
  node r;
  if (v.size() == 0) r = Lit(1);
  else r = ReduceInternal<node (*)(node, node)>(v, And)[0];
  HIERARCHY_EXIT();
  return r;
}

node operator==(vector<node> a, vector<node> b) {
  HIERARCHY_ENTER();
  node r;
  if (a.size() != b.size()) {
    r = Lit(0);
  } else {
    vector<node> eq;
    for (unsigned i = 0; i < a.size(); ++i) eq.push_back(a[i] == b[i]);
    r = AndN(eq);
  }
  HIERARCHY_EXIT();
  return r;
}

// One bit of a pipeline register
struct pregbit {
  pregbit(node d, node q): d(d), q(q) {}
  node d, q;
};

// Entire pipeline register
struct preg {
  vector<node> stall, flush, bubble;
  vector<pregbit> bits;
  node anystall;
};

map<unsigned, preg> pregs;

node PipelineReg(unsigned n, node d) {
  node q;
  pregs[n].bits.push_back(pregbit(d, q));
  return q;
}

void PipelineStall(unsigned n, node stall) {
  for (unsigned i = 0; i < n; ++i) pregs[i].stall.push_back(stall);
  pregs[n].bubble.push_back(stall);
}

node GetStall(unsigned n) {
  return pregs[n].anystall;
}

void PipelineFlush(unsigned n, node flush) {
  for (unsigned i = 0; i <= n; ++i) pregs[i].flush.push_back(flush);
}

template <unsigned N> bvec<N> PipelineReg(unsigned n, bvec<N> d) {
  bvec<N> q;
  for (unsigned i = 0; i < N; ++i) q[i] = PipelineReg(n, d[i]);
  return q;
}

struct hazdst_t {
  unsigned stage;
  node valid;
  vector<node> bypassIn, output, name;
};

struct hazsrc_t {
  unsigned stage;
  vector<node> value, name;
  node valid, ready;
};

struct hazsig_t {
  unsigned stage;
  vector<node> name;
  node valid;
};

vector<hazdst_t> hazdsts;
vector<hazsrc_t> hazsrcs;
vector<hazsig_t> hazsigs;

// Hazard signal. Used to signal a hazard for which forwarding is not possible.
// Not used in the SPIMlike pipeline.
template <unsigned M>
  void HazSig(unsigned stage, node vld, bvec<M> name)
{
  hazsig_t h;
  for (unsigned i = 0; i < M; ++i) h.name.push_back(name[i]);
  h.stage = stage;
  h.valid = vld;
  hazsigs.push_back(h);
}

// Hazard source; may be valid.
template <unsigned M, unsigned N>
  void HazSrc(unsigned stage, node rdy, node vld, bvec<N> value, bvec<M> name)
{
  hazsrc_t h;
  h.stage = stage;
  for (unsigned i = 0; i < M; ++i) h.name.push_back(name[i]);
  for (unsigned i = 0; i < N; ++i) h.value.push_back(value[i]);
  h.valid = vld;
  h.ready = rdy;

  tap(string("hsrc_") + "dxmwz"[stage] + "_name", name);
  tap(string("hsrc_") + "dxmwz"[stage] + "_value", value);
  tap(string("hsrc_") + "dxmwz"[stage] + "_valid", vld);
  tap(string("hsrc_") + "dxmwz"[stage] + "_ready", rdy);

  hazsrcs.push_back(h);
}

// Hazard destination
template <unsigned M, unsigned N>
bvec<N> HazDst(unsigned stage, node vld, bvec<N> passThru, bvec<M> name)
{
  bvec<N> out;
  hazdst_t h;
  for (unsigned i = 0; i < N; ++i) {
    h.bypassIn.push_back(passThru[i]);
    h.output.push_back(out[i]);
  }
  for (unsigned i = 0; i < M; ++i) h.name.push_back(name[i]);
  h.stage = stage;
  h.valid = vld;

  tap(string("hdst_") + char('0' + hazdsts.size()) + "_name", name);
  tap(string("hdst_") + char('0' + hazdsts.size()) + "_passThru", passThru);
  tap(string("hdst_") + char('0' + hazdsts.size()) + "_out", out);
  tap(string("hdst_") + char('0' + hazdsts.size()) + "_valid", vld);

  hazdsts.push_back(h);

  return out;
}

void genPipelineRegs() {
  // Order the hazard sources.
  multimap<unsigned, unsigned> s;
  vector<hazsrc_t> srcs;
  for (unsigned i = 0; i < hazsrcs.size(); ++i)
    s.insert(pair<unsigned, unsigned>(hazsrcs[i].stage, i));
  for (auto it = s.begin(); it != s.end(); ++it)
    srcs.push_back(hazsrcs[it->second]);
  hazsrcs = srcs;

  // Generate the hazard/forwarding unit.
  for (unsigned i = 0; i < hazdsts.size(); ++i) {
    bvec<32> fwdsigs(Lit<32>(1));

    for (unsigned j = 0; j < hazsrcs.size(); ++j) {
      node haz(hazdsts[i].name == hazsrcs[j].name
               && hazdsts[i].valid && hazsrcs[j].valid),
           fwd(haz && hazsrcs[j].ready),
           stall(haz && !hazsrcs[j].ready);

      fwdsigs[hazsrcs.size() - j] = fwd;

      tap(string("haz_")+"dxmwz"[hazsrcs[j].stage]+ "to"+char('0'+i), haz);
      tap(string("fwd_")+"dxmwz"[hazsrcs[j].stage]+"to"+char('0'+i), fwd);
      tap(string("stall_")+"dxmwz"[hazsrcs[j].stage]+"to"+char('0'+i), stall);

      PipelineStall(hazdsts[i].stage + 1, stall);
    }

    vec<32, bvec<64>> values;
    for (unsigned j = 0; j < hazdsts[i].bypassIn.size(); ++j)
      values[0][j] = hazdsts[i].bypassIn[j];
    for (unsigned i = 0; i < hazsrcs.size(); ++i)
      for (unsigned j = 0; j < hazsrcs[i].value.size(); ++j)
        values[hazsrcs.size() - i][j] = hazsrcs[i].value[j];

    bvec<5> fwdsel(Log2(fwdsigs));
    bvec<64> val(Mux(fwdsel, values));
    for (unsigned j = 0; j < hazdsts[i].output.size(); ++j)
      hazdsts[i].output[j] = val[j];

    tap(string("fwdsel")+char('0'+i), fwdsel[range<0,1>()]);
  }

  

  // Create the pipeline registers
  for (auto it = pregs.begin(); it != pregs.end(); ++it) {
    ostringstream oss; oss << it->first;
    hierarchy_enter("pipeline_reg" + oss.str());
    preg &pr(it->second);
    pr.anystall = OrN(pr.stall);
    for (unsigned i = 0; i < pr.bits.size(); ++i)
      pr.bits[i].q = Wreg(
        !pr.anystall,
        Mux(OrN(pr.flush) || OrN(pr.bubble), pr.bits[i].d, Lit(0))
      );
    hierarchy_exit();
  }
}

// Types describing read and write ports
template <unsigned M, unsigned N> struct rdport {
  rdport() {}
  rdport(bvec<M> a, bvec<N> q): a(a), q(q) {}
  bvec<M> a;
  bvec<N> q;
};

template <unsigned M, unsigned N> struct wrport {
  wrport() {}
  wrport(bvec<M> a, bvec<N> d, node we): a(a), d(d), we(we) {}
  bvec<M> a;
  bvec<N> d;
  node we;
};

// 2^M-entry N-bit R-read-port, 1-write-port register file
template <unsigned M, unsigned N, unsigned R>
  void Regfile(vec<R, rdport<M, N>> r, wrport<M, N> w)
{
  HIERARCHY_ENTER();
  const unsigned long SIZE(1<<M);

  bvec<SIZE> wrsig;
  wrsig = Decoder(w.a, w.we);

  vec<SIZE, bvec<N>> regs;
  for (unsigned i = 0; i < SIZE; ++i) {
    regs[i] = Wreg(wrsig[i], w.d);
    ostringstream oss;
    oss << "reg" << i;
    tap(oss.str(), regs[i]);
  }

  for (unsigned i = 0; i < R; ++i)
    r[i].q = Mux(w.we && w.a == r[i].a, Mux(r[i].a, regs), w.d);
  HIERARCHY_EXIT();
}

// For the implementation of the LUI instruction, move lower N/2 bits of input
// to upper half of output.
template <unsigned N> bvec<N> ToUpper(bvec<N> in) {
  bvec<N> out;
  out[range<  0, N/2-1>()] = Lit<N/2>(0);
  out[range<N/2,   N-1>()] = in[range<0, N/2-1>()];
  return out;
}

// This ALU can perform the following operations:
//
//    Opcode | Operation              Opcode | Operation
//   --------+--------------------   --------+--------------------
//   0x0-0x3 | ToUpper (For LUI)     0x4-0x5 | Left Shift
//     0x6   | Right shift             0x7   | Right shift arithmetic
//   0x8-0x9 | Add                   0xa-0xb | Subtract
//     0xc   | Bitwise AND             0xd   | Bitwise OR
//     0xe   | Bitwise XOR             0xf   | Bitwise NOR
template <unsigned M> bvec<1<<M> Alu(bvec<4> opsel, bvec<1<<M> a, bvec<1<<M> b)
{
  HIERARCHY_ENTER();
  bvec<1<<M> subbit;
  for (unsigned i = 0; i < (1<<M); ++i) subbit[i] = opsel[1];

  vec<4, bvec <1<<M> > logicResult;
  logicResult[0] = a & b;
  logicResult[1] = a | b;
  logicResult[2] = a ^ b;;
  logicResult[3] = ~(a | b);
  
  bvec<1<<M> logic = Mux(opsel[range<0,1>()], logicResult);

  vec<4, bvec<1<<M> > aluResult;
  bvec<M> shamt = a[range<0,M-1>()];
  aluResult[0] = ToUpper(b);
  aluResult[1] = Shifter(b, shamt, opsel[0], opsel[1]);
  aluResult[2] = Adder(a, (b ^ subbit), opsel[1]);
  aluResult[3] = logic;

  bvec<1<<M> r(Mux(opsel[range<2,3>()], aluResult));
  HIERARCHY_EXIT();
  return r;
}

void pipeline() {
  // // // Fetch stage // // //
  hierarchy_enter("Fetch");
  bvec<32> pcplus4_f, brtarg_x;
  node taken_br_x; 
  bvec<32> pc_f(Wreg(!GetStall(0), Mux(taken_br_x, pcplus4_f, brtarg_x)));
  TAP(pc_f);

  bvec<32> iramq_f(LLRom<6,32>(pc_f[range<2, 7>()], "sieve.hex"));

  pcplus4_f = pc_f + Lit<32>(4);

  // // // F->D Pipeline Regs // // //
  bvec<32> pcplus4_d(PipelineReg(0, pcplus4_f));
  bvec<32> iramq_d  (PipelineReg(0, iramq_f));
  hierarchy_exit();

  // // // Decode stage // // //
  hierarchy_enter("Decode");
  bvec<32> sext_imm_d(Sext<32>(iramq_d[range<0, 15>()]));

  bvec<6> opcode(iramq_d[range<26, 31>()]), func(iramq_d[range<0, 5>()]);
  bvec<8> itype_d;
  itype_d[0] = Lit<6>(0x2b) == opcode;
  itype_d[1] = Lit<6>(0x23) == opcode;
  itype_d[2] = !opcode[5] && opcode[3];
  itype_d[3] = Lit<6>(0) == opcode && (func[5] || func[2]);
  itype_d[4] = Lit<6>(0) == opcode && !(func[5] || func[2]);
  itype_d[5] = Lit<5>(0x02) == bvec<5>(opcode[range<1, 5>()]);
  itype_d[6] = Lit(0);
  itype_d[7] = Lit(0);

  node taken_br_m,
       nbr(!(taken_br_x || taken_br_m)), 
       wb_d(!(itype_d[0] || itype_d[5]) && nbr),
       br_d((itype_d[5] && nbr));

  node rdsrc1_d(!(itype_d[1] || itype_d[2])), rdsrc0_d(!itype_d[4]);

  node dsel(itype_d[0] || itype_d[1] || itype_d[2]);
  bvec<5> sidx0_d(iramq_d[range<21,25>()]), sidx1_d(iramq_d[range<16,20>()]),
          didx_d(Mux(dsel, iramq_d[range<11,15>()], iramq_d[range<16,20>()]));

  bvec<32> rfa_d, rfb_d;

  // Predeclaring the write port for the writeback stage.
  node wb_w; bvec<5> didx_w; bvec<32> wbval_w;

  // The register file and its ports.
  vec<2, rdport<5, 32>> rf_rd;
  rf_rd[0] = rdport<5, 32>(sidx0_d, rfa_d);
  rf_rd[1] = rdport<5, 32>(sidx1_d, rfb_d);
  wrport<5, 32> rf_wr0(didx_w, wbval_w, wb_w);
  Regfile(rf_rd, rf_wr0);

  bvec<8> itype_x;
  node wb_x;
  bvec<5> didx_x;

  node justadd_d(itype_d[0] || itype_d[1]), wrmem_d(itype_d[0]);

  bvec<4> opsel0_d, opsel1_d;

  node usefunc_d(itype_d[3] || itype_d[4]);
  opsel0_d[0] = func[0] && usefunc_d || opcode[0];
  opsel0_d[1] = func[1] && usefunc_d || opcode[1];
  opsel0_d[2] = func[2] && usefunc_d || itype_d[4] || opcode[2];
  opsel0_d[3] = func[5] && usefunc_d || opcode[3];

  opsel1_d = Mux(opcode == Lit<6>(0x0f), opsel0_d, Lit<4>(0));
  bvec<4> opsel_d(Mux(justadd_d, opsel1_d, Lit<4>(8)));

  bvec<32> brtarg_d(
    pcplus4_d + Cat(sext_imm_d[range<0,29>()], Lit<2>(0))
  );

  // // // D->X Pipeline Regs // // //
  node wrmem_x(PipelineReg(1, wrmem_d)),
       br_x   (PipelineReg(1, br_d)),
       bne_x  (PipelineReg(1, iramq_d[26]));
  itype_x  =   PipelineReg(1, itype_d);
  wb_x     =   PipelineReg(1, wb_d);
  didx_x   =   PipelineReg(1, didx_d);
  brtarg_x =   PipelineReg(1, brtarg_d);
  didx_x   =   PipelineReg(1, didx_d);
  bvec<5> sidx0_x(PipelineReg(1, sidx0_d)),
          sidx1_x(PipelineReg(1, sidx1_d));
  bvec<32> sext_imm_x(PipelineReg(1, sext_imm_d)), 
           rfa_x(PipelineReg(1, rfa_d)),
           rfb_x(PipelineReg(1, rfb_d));
  bvec<4> opsel_x(PipelineReg(1, opsel_d));
  node    rdsrc0_x(PipelineReg(1, rdsrc0_d)),
          rdsrc1_x(PipelineReg(1, rdsrc1_d));

  hierarchy_exit();

  // // // Execute stage // // //
  hierarchy_enter("Execute");

  node wb_m;
  bvec<5> didx_m;
  bvec<32> aluval_m;

  bvec<32> rfa_fd_x(HazDst(1, rdsrc0_x, rfa_x, sidx0_x)),
           rfb_fd_x(HazDst(1, rdsrc1_x, rfb_x, sidx1_x));

  TAP(sidx0_x);  TAP(sidx1_x);
  TAP(rfa_x);    TAP(rfb_x);
  TAP(rfa_fd_x); TAP(rfb_fd_x);

  // The ALU
  bvec<32> aluval_x = Alu<5>(
    opsel_x, // The first mux selects between reg val and shift amount.
    Mux(itype_x[4], rfa_fd_x,
        Cat(Lit<27>(0), sext_imm_x[range<6, 10>()])),
    Mux(Nor(itype_x[3], itype_x[4]), rfb_fd_x, sext_imm_x)
  );

  // Taken branch?
  taken_br_x = Xor(rfa_fd_x == rfb_fd_x, bne_x) && br_x;

  // X->M pipeline regs
  wb_m          = PipelineReg(2, wb_x);
  didx_m        = PipelineReg(2, didx_x);
  aluval_m      = PipelineReg(2, aluval_x);
  taken_br_m    = PipelineReg(2, taken_br_x);
  node    wrmem_m(PipelineReg(2, wrmem_x)),
          rdmem_m(PipelineReg(2, itype_x[1]));
  bvec<32>  rfb_m(PipelineReg(2, rfb_fd_x));
  hierarchy_exit();

  // // // Memory stage // // //
  hierarchy_enter("Memory");
  // The ALU output from the previous stage is a source for forwarding if the
  // instruction in this stage is not a memory instruction.
  HazSrc(2, !rdmem_m, wb_m, aluval_m, didx_m);

  // Data RAM
  bvec<32> memq_w(Syncmem(aluval_m[range<2,19>()], rfb_m, wrmem_m));

  // M->W pipeline regs
  bvec<32> aluval_w(PipelineReg(3, aluval_m));
  node rdmem_w = PipelineReg(3, rdmem_m);
  wb_w = PipelineReg(3, wb_m);
  didx_w = PipelineReg(3, didx_m);

  hierarchy_exit();

  // // // Writeback stage // // //
  hierarchy_enter("Writeback");

  wbval_w = Mux(rdmem_w, aluval_w, memq_w);
  HazSrc(3, Lit(1), wb_w, wbval_w, didx_w);

  hierarchy_exit();

  genPipelineRegs();
}

int main() {
  pipeline();

  //if (cycdet()) {
  //  cerr << "Cycle detected in logic DAG. Exiting." << endl;
  //  return 1;
  //}

  optimize();

  cerr << "Critical path: " << critpath() << endl;

  // Do the simulation
  ofstream wave_file("example6.vcd");
  run(wave_file, 1000);

  // Print the netlist
  ofstream netlist_file("example6.nand");
  print_netlist(netlist_file);

  // Print a dot file.
  ofstream dot_file("example6.dot");
  print_dot(dot_file);

  print_hierarchy();

  return 0;
}
