#ifndef REPORT_H
#define REPORT_H

#include <iostream>

static void report() {
  using namespace std;
  using namespace chdl;

  cerr << "Final design size: " << endl
       << "  Inverters  : " << dec << num_inverters() << endl
       << "  Nand Gates  : " <<  num_nands() << endl
       << "  D Flip-Flops: " << num_regs() << endl
       << "  SRAM bits   : " << num_sram_bits() << endl;
}

#endif
