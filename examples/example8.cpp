// This example implements a simple state machine, a string matcher.
#include <iostream>
#include <fstream>

#include <statemachine.h>

#include <lit.h>

#include <llmem.h>

#include <tap.h>
#include <analysis.h>
#include <vis.h>
#include <sim.h>
#include <opt.h>
#include <netlist.h>

using namespace chdl;
using namespace std;

template <unsigned N> node match(bvec<N> a, unsigned b, unsigned c) {
  return a == Lit<N>(b) || a == Lit<N>(c);
}

int main(int argc, char **argv) {
  bvec<6> counter;
  counter = Reg(counter + Lit<6>(1));
  bvec<7> charinput = LLRom<6, 7>(counter, "text.hex");

  // State:
  //   0 - no letters seen
  //   1 - "C" seen
  //   2 - "H" seen
  //   3 - "D" seen
  //   4 - "L" seen -- successful match.
  //
  // Since this machine stays in the '"L" seen' state for 1 cycle, it cannot
  // handle back-to-back matches. CHDLCHDL would lead to only a single match.
  node cMatch(match(charinput, 'c', 'C')), hMatch(match(charinput, 'h', 'H')),
       dMatch(match(charinput, 'd', 'D')), lMatch(match(charinput, 'l', 'L'));

  Statemachine<5> sm;
  sm.edge(0, 1, cMatch);  sm.edge(1, 2, hMatch);
  sm.edge(2, 3, dMatch);  sm.edge(3, 4, lMatch);

  sm.edge(0, 0, !cMatch); sm.edge(1, 0, !hMatch);
  sm.edge(2, 0, !dMatch); sm.edge(3, 0, !lMatch);

  sm.edge(4, 0, Lit(1));

  sm.generate();

  bvec<CLOG2(5)> state(sm);

  node match(state == Lit<CLOG2(5)>(4));

  TAP(charinput);
  TAP(state);
  TAP(match);

  optimize();

  ofstream dot_file("example8.dot");
  print_dot(dot_file);
  dot_file.close();

  if (cycdet()) {
    cerr << "Cycle detected. Exiting." << endl;
    return 1;
  }

  ofstream wave_file("example8.vcd");
  run(wave_file, 80);
  
  ofstream netlist_file("example8.nand");
  print_netlist(netlist_file);

  return 0;
}
