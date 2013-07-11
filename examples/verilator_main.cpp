#include "Vexample2.h"

#include <verilated.h>
#include <verilated_vcd_c.h>

#include <iostream>

#include <sys/time.h>

using namespace std;

unsigned long time_us() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return 1000000ul * tv.tv_sec + tv.tv_usec;
}

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
  Verilated::traceEverOn(true);
<<<<<<< HEAD
  Vexample6 *top = new Vexample6;
=======
  Vexample2 *top = new Vexample2;
>>>>>>> 6675bb9f977ccc7df773b7ca23cd66819dc135b2
  VerilatedVcdC trace_obj;


  unsigned long main_time(0), start(time_us());
  top->trace(&trace_obj, 1);
  trace_obj.open("dump.vcd");
  while (main_time < 2000) {
    top->phi = !top->phi;
    top->eval();
    trace_obj.dump(main_time);
    ++main_time;
  }
  trace_obj.close();

  unsigned long duration(time_us() - start);

  cerr << duration/1000.0 << "ms\n";

  delete top;
  return 0;
}
