// This example is a test/demonstrator of the register file.
#include <iostream>
#include <sstream>
#include <fstream>

#include <gateops.h>
#include <bvec-basic-op.h>
#include <adder.h>
#include <mux.h>
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
    rvec<32> r(Reg<32>());
    regs[i] = r;  //Wreg(wrsig[i], wval);
    bvec<32> next(Mux(wrsig[i], regs[i], wval));
    r.connect(next);

    ostringstream oss;
    oss << "reg" << i;
    tap<32, rvec>(oss.str(), regs[i]);
    tap<32, bvec>(oss.str() + "n", next);
    oss << "w";
    tap(oss.str(), wrsig[i]);
  }

  r0val = Mux(r0idx, regs);
  r1val = Mux(r1idx, regs);
}

bvec<32> Sext(bvec<16> in) {
  bvec<32> r;
  r[range<0,15>()] = in;
  for (unsigned i = 16; i < 32; ++i) r[i] = in[15];
  return r;
}

int main(int argc, char **argv) {
  rvec<5> sidx0(Reg<5>());
  bvec<5> sidx1(~sidx0),
          didx((sidx0 ^ Lit<5>(0x15)) & Lit<5>(0x1e));
  rvec<32> wrval(Reg<32>());

  sidx0.connect(sidx0 + Lit<5>(1));
  wrval.connect(wrval + Lit<32>(1));
  
  TAP(sidx0); TAP(sidx1); TAP(didx); TAP(wrval);

  node w = wrval[6] && wrval[2] || wrval[3] && wrval[0];
  TAP(w);

  bvec<32> rdval0, rdval1;
  Regfile(rdval0, rdval1, sidx0, sidx1, didx, w, wrval);
  TAP(rdval0); TAP(rdval1);

  // The simulation (generate .vcd file)
  optimize();
  run(cout, 1000);

  ofstream netlist_file("example7.nand");
  print_netlist(netlist_file);
  netlist_file.close();
}
