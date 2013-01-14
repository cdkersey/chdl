// This example implements a simple pipelined harvard architecture CPU with a
// SPIM-like ISA.
#include <iostream>
#include <sstream>

#include <gateops.h>
#include <bvec-basic-op.h>
#include <adder.h>
#include <mux.h>
#include <memory.h>

#include <opt.h>
#include <tap.h>
#include <sim.h>

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
    tap<32, rvec>(oss.str(), regs[i]);
  }

  r0val = Mux(r1idx, regs);
  r1val = Mux(r0idx, regs);
}

bvec<32> Sext(bvec<16> in) {
  bvec<32> r;
  r[range<0,15>()] = in;
  for (unsigned i = 16; i < 32; ++i) r[i] = in[15];
  return r;
}

int main(int argc, char **argv) {
  rvec<32> pc_f;
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
  reg wb_m(Reg()), taken_br_m(Reg()), rdmem_m(Reg());
  TAP(wb_m);

  rvec<32> wbval_w(Reg<32>()); TAP(wbval_w);
  rvec<5> didx_w(Reg<5>()); TAP(didx_w);
  reg wb_w(Reg()); TAP(wb_w);

  bvec<32> iramq_f(Rom<18,32>(pc_f[range<2, 19>()], "sieve.hex"));
  TAP(iramq_f);

  bvec<32> pcplus4_f(pc_f + Lit<32>(4));

  bvec<32> sext_imm_d(Sext(iramq_d[range<0,15>()]));

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

  bvec<32> rfa_d, rfb_d, rfa0_d, rfb0_d;
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

  // // // Execute stage // // //
  

  // The simulation (generate .vcd file)
  optimize();
  run(cout, 65536);
}
