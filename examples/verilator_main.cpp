#include "Vexample2.h"

#include <verilated.h>
#include <verilated_vcd_c.h>

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
  Verilated::traceEverOn(true);
  Vexample2 *top = new Vexample2;
  VerilatedVcdC trace_obj;

  unsigned long main_time(0);  
  top->trace(&trace_obj, 1);
  trace_obj.open("dump.vcd");
  while (main_time < 2000) {
    top->phi = !top->phi;
    top->eval();
    trace_obj.dump(main_time);
    ++main_time;
  }

  trace_obj.close();

  delete top;
  return 0;
}
