// This example implements a simple pipelined harvard architecture CPU with a
// SPIM-like ISA.
#include <iostream>
#include <fstream>
#include <sstream>

#include <gateops.h>
#include <bvec-basic-op.h>
#include <adder.h>
#include <mux.h>
#include <llmem.h>
#include <memory.h>

#include <opt.h>
#include <tap.h>
#include <sim.h>
#include <netlist.h>

using namespace std;
using namespace chdl;

// 32x32 2-port register file
void Regfile(bvec<32> &r0val, bvec<32> &r1val, bvec<5> r0idx, bvec<5> r1idx,
             bvec<5> widx, node w, bvec<32> wval)
{
  bvec<32> wrsig = Decoder(widx, w);
  vec<32, bvec<32>> regs;
  for (unsigned i = 0; i < 32; ++i) {
    regs[i] = Wreg(wrsig[i], wval);
    ostringstream oss;
    oss << "reg" << i;
    tap<32, bvec>(oss.str(), regs[i]);
  }

  r0val = Mux(r0idx, regs);
  r1val = Mux(r1idx, regs);
}

// Fixed shift by B bits (positive for left, negative for right). A series of
// these could be used to construct a barrel shifter.
template <unsigned N>
  bvec<N> ShifterStage(int B, bvec<N> in, node enable, node arith)
{
  bvec<N> shifted;
  for (int i = 0; i < N; ++i) {
    if (i+B >= 0 && i+B < N) shifted[i] = in[i+B];
    else if (B > 0)          shifted[i] = And(in[N-1], arith);
    else                     shifted[i] = Lit(0);
  }

  return Mux(enable, in, shifted);
}

// 2^M bit bidirectional barrel shifter.
template <unsigned M>
  bvec<1<<M> Shifter(bvec<1<<M> in, bvec<M> shamt, node arith, node dir)
{
  vector<bvec<1<<M> > vl(M+1), vr(M+1);
  vl[0] = vr[0] = in;

  for (unsigned i = 0; i < M; ++i) {
    vl[i+1] = ShifterStage<1<<M>(-(1<<i), vl[i], shamt[i], arith);
    vr[i+1] = ShifterStage<1<<M>( (1<<i), vr[i], shamt[i], arith);
  }

  return Mux(dir, vl[M], vr[M]);
}


// For the implementation of the LUI instruction, move lower N/2 bits of input
// to upper half of output.
template <unsigned N> bvec<N> ToUpper(bvec<N> in) {
  bvec<N> out;
  out[range<  0, N/2-1>()] = Lit<N/2>(0);
  out[range<N/2,   N-1>()] = in[range<0, N/2-1>()];
  return out;
}

// The ALU can perform the following operations:
//
//    Opcode | Operation
//   --------+--------------------
//   0x0-0x3 | ToUpper (For LUI)
//   0x4-0x5 | Left Shift
//     0x6   | Right shift
//     0x7   | Right shift arithmetic
//   0x8-0x9 | Add 
//   0xa-0xb | Subtract
//     0xc   | Bitwise AND
//     0xd   | Bitwise OR
//     0xe   | Bitwise XOR
//     0xf   | Bitwise NOR
//
// It should come as no surprise that the table of operations
// requires more lines of code than the implementation.
template <unsigned M> bvec<1<<M> Alu(bvec<4> opsel, bvec<1<<M> a, bvec<1<<M> b)
{
  tap<32, bvec>("aluin_a", a);
  tap<32, bvec>("aluin_b", b);

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

  TAP(a); TAP(b); TAP(opsel);
  bvec<32> rval(Mux(opsel[range<2,3>()], aluResult));
  TAP(rval);

  return rval;
}

int main(int argc, char **argv) {
  rvec<32> pc_f(Reg<32>());
  TAP(pc_f);

  rvec<32> pcplus4_d(Reg<32>()), iramq_d(Reg<32>());

  rvec<32> sext_imm_x(Reg<32>()), rfa_x(Reg<32>()),
           rfb_x(Reg<32>()), brtarg_x(Reg<32>());
  rvec<8> itype_x(Reg<8>());
  rvec<5> sidx0_x(Reg<5>()), sidx1_x(Reg<5>()), didx_x(Reg<5>());
  rvec<4> opsel_x(Reg<4>());
  reg br_x(Reg()), wb_x(Reg()), bne_x(Reg()), wrmem_x(Reg());
  TAP(wb_x);

  rvec<32> rfb_m(Reg<32>()), aluval_m(Reg<32>());
  rvec<5> didx_m(Reg<5>());
  reg wb_m(Reg()), taken_br_m(Reg()), wrmem_m(Reg()), rdmem_m(Reg());
  TAP(wb_m);

  rvec<5> didx_w(Reg<5>()); TAP(didx_w);
  reg wb_w(Reg()); TAP(wb_w);

  bvec<32> iramq_f(LLRom<6,32>(pc_f[range<2, 7>()], "sieve.hex"));
  TAP(iramq_f);

  bvec<32> pcplus4_f(pc_f + Lit<32>(4));

  bvec<32> sext_imm_d(Sext<32>(iramq_d[range<0,15>()]));

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

  TAP(opcode);
  TAP(itype_d);
  TAP(itype_x);

  node rdsrc1_d(!(itype_d[1] || itype_d[2])), rdsrc0_d(!itype_d[4]);

  node dsel(itype_d[0] || itype_d[1] || itype_d[2]);
  bvec<5> sidx0_d(iramq_d[range<21,25>()]), sidx1_d(iramq_d[range<16,20>()]),
          didx_d(Mux(dsel, bvec<5>(iramq_d[range<11,15>()]),
                           bvec<5>(iramq_d[range<16,20>()])));

  bvec<32> rfa_d, rfb_d, rfa0_d, rfb0_d, wbval_w(Lit<32>(0xdeadbeef));
  Regfile(rfa0_d, rfb0_d, sidx0_d, sidx1_d, didx_w, wb_w, wbval_w);

  rfa_d = Mux(wb_w && didx_w == sidx0_d, rfa0_d, wbval_w);
  rfb_d = Mux(wb_w && didx_w == sidx1_d, rfb0_d, wbval_w);

  node bubble(itype_x[1] && (sidx0_d == didx_x && rdsrc0_d ||
                             sidx1_d == didx_x && rdsrc1_d) && wb_x);
  TAP(rdsrc0_d); TAP(rdsrc1_d);

  node nbubble(!bubble), justadd_d(itype_d[0] || itype_d[1]),
       wrmem_d(itype_d[0] && nbubble);

  bvec<4> opsel0_d, opsel1_d;

  node usefunc_d(itype_d[3] || itype_d[4]);
  opsel0_d[0] = func[0] && usefunc_d || opcode[0];
  opsel0_d[1] = func[1] && usefunc_d || opcode[1];
  opsel0_d[2] = func[2] && usefunc_d || itype_d[4] || opcode[2];
  opsel0_d[3] = func[5] && usefunc_d || opcode[3];

  opsel1_d = Mux(opcode == Lit<6>(0x0f), opsel0_d, Lit<4>(0));
  bvec<4> opsel_d(Mux(justadd_d, opsel1_d, Lit<4>(8)));

  bvec<32> brtarg_d(
    pcplus4_d + Cat(bvec<30>(sext_imm_d[range<0,29>()]), Lit<2>(0))
  );
  TAP(brtarg_d); TAP(brtarg_x);

  // // // Execute stage // // //
  bvec<2> fwdsel_0, fwdsel_1;

  fwdsel_0[0] = wb_m && didx_m == sidx0_x;
  fwdsel_0[1] = wb_w && didx_w == sidx0_x && didx_m != sidx0_x;

  fwdsel_1[0] = wb_m && didx_m == sidx1_x;
  fwdsel_1[1] = wb_w && didx_w == sidx1_x && didx_m != sidx1_x;

  vec<4, bvec<32> > rfa_fd_mux_in, rfb_fd_mux_in;

  rfa_fd_mux_in[0] = rfa_x;
  rfa_fd_mux_in[1] = aluval_m;
  rfa_fd_mux_in[2] = wbval_w;
  rfa_fd_mux_in[3] = Lit<32>(0);

  rfb_fd_mux_in[0] = rfb_x;
  rfb_fd_mux_in[1] = aluval_m;
  rfb_fd_mux_in[2] = wbval_w;
  rfb_fd_mux_in[3] = Lit<32>(0);

  bvec<32> rfa_fd_x(Mux(fwdsel_0, rfa_fd_mux_in)),
           rfb_fd_x(Mux(fwdsel_1, rfb_fd_mux_in));
  TAP(fwdsel_0); TAP(fwdsel_1);

  // The ALU
  bvec<32> aluval_x = Alu<5>(
    opsel_x, // The first mux selects between reg val and shift amount.
    Mux(itype_x[4], rfa_fd_x,
        Cat(Lit<27>(0), bvec<5>(sext_imm_x[range<6, 10>()]))),
    Mux(Nor(itype_x[3], itype_x[4]), rfb_fd_x, sext_imm_x)
  );

  // Taken branch?
  node taken_br_x(Xor(rfa_fd_x == rfb_fd_x, bne_x) && br_x);
  TAP(taken_br_x);

  // // // Memorystage // // //
  // Data RAM
  rvec<32> aluval_w(Reg<32>());
  aluval_w.connect(aluval_m);

  reg rdmem_w(Reg()); rdmem_w.connect(rdmem_m);
  
  bvec<32> memq_w(Syncmem(bvec<18>(aluval_m[range<2,19>()]), rfb_m, wrmem_m));
  wbval_w = Mux(rdmem_w, aluval_w, memq_w);
  TAP(wbval_w); TAP(memq_w); TAP(aluval_m); TAP(aluval_x);
  TAP(rdmem_m); TAP(wrmem_m); TAP(rfb_m);

  // // // Final signals // // //
  // These signals require all of the other pipeline signals to have already
  // been defined.
  node nbr_and_nbubble = nbubble && !(taken_br_x || taken_br_m),
       wb_d = !(itype_d[0] || itype_d[5]) && nbr_and_nbubble,
       br_d = (itype_d[5] && nbr_and_nbubble);
  TAP(bubble);
  TAP(wb_d);
  TAP(br_d);

  // // // Register wireups // // //
  // Program counter d/w
  Wreg(pc_f, Mux(taken_br_x, pcplus4_f, brtarg_x), nbubble);

  // F->D pipeline regs
  Wreg(pcplus4_d, pcplus4_f, nbubble);
  Wreg(iramq_d, iramq_f, nbubble);

  // D->X pipeline regs
  wrmem_x.connect(wrmem_d);
  itype_x.connect(itype_d);
  br_x.connect(br_d);
  wb_x.connect(wb_d);
  sidx0_x.connect(sidx0_d);
  sidx1_x.connect(sidx1_d);
  didx_x.connect(didx_d);
  sext_imm_x.connect(sext_imm_d);
  rfa_x.connect(rfa_d);
  rfb_x.connect(rfb_d);
  opsel_x.connect(opsel_d);
  bne_x.connect(iramq_d[26]);
  brtarg_x.connect(brtarg_d);

  // X->M pipeline regs
  wb_m.connect(wb_x);
  didx_m.connect(didx_x);
  aluval_m.connect(aluval_x);
  taken_br_m.connect(taken_br_x);
  wrmem_m.connect(wrmem_x);
  rdmem_m.connect(itype_x[1]);
  rfb_m.connect(rfb_fd_x);

  // M->W pipeline regs
  wb_w.connect(wb_m);
  didx_w.connect(didx_m);

  TAP(iramq_d);
  TAP(sidx0_d); TAP(sidx1_d);
  TAP(didx_d); TAP(didx_x); TAP(didx_m);
  TAP(sext_imm_d);
  TAP(sext_imm_x);

  TAP(rfa_d); TAP(rfb_d);
  TAP(rfa_x); TAP(rfb_x);

  // The simulation (generate .vcd file)
  optimize();
  run(cout, 1000);

  ofstream netlist_file("example6.nand");
  print_netlist(netlist_file);
  netlist_file.close();
}
