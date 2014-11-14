// This example implements a simple state machine, a string matcher.
#include <iostream>
#include <fstream>

#include <gateops.h>
#include <lit.h>

#include <llmem.h>

#include <bvec-basic-op.h>
#include <adder.h>
#include <tap.h>
#include <analysis.h>
#include <vis.h>
#include <sim.h>
#include <opt.h>
#include <netlist.h>
#include <hierarchy.h>
#include <cassign.h>
#include <mux.h>

#include "report.h"

using namespace chdl;
using namespace std;

template <unsigned N> node match(bvec<N> a, unsigned b, unsigned c) {
  return a == Lit<N>(b) || a == Lit<N>(c);
}

int main(int argc, char **argv) {
  bvec<6> counter;
  counter = Reg(counter + Lit<6>(1));
  bvec<7> charinput = LLRom<6, 7>(counter, "text.hex");

  // match goes high for 1 cycle for each instance of the text "CHDL" in any
  // capitalization in the input stream.
  node cMatch(match(charinput, 'c', 'C')), hMatch(match(charinput, 'h', 'H')),
       dMatch(match(charinput, 'd', 'D')), lMatch(match(charinput, 'l', 'L'));

  enum state { INITIAL, C_SEEN, H_SEEN, D_SEEN, L_SEEN, N_STATES };
  bvec<CLOG2(N_STATES)> next_state, state(Reg(next_state));
  bvec<N_STATES> state_1h(Zext<N_STATES>(Decoder(state)));
  Cassign(next_state).
    IF((state_1h[INITIAL] || state_1h[L_SEEN]) && cMatch,
         Lit<CLOG2(N_STATES)>(C_SEEN)).
    IF(state_1h[C_SEEN] && hMatch, Lit<CLOG2(N_STATES)>(H_SEEN)).
    IF(state_1h[H_SEEN] && dMatch, Lit<CLOG2(N_STATES)>(D_SEEN)).
    IF(state_1h[D_SEEN] && lMatch, Lit<CLOG2(N_STATES)>(L_SEEN)).
    ELSE(Lit<CLOG2(N_STATES)>(INITIAL));

  node match(state == Lit<CLOG2(N_STATES)>(L_SEEN));

  TAP(charinput);
  TAP(state);
  TAP(match);

  optimize();

  ofstream dot_file("example8.dot");
  dot_schematic(dot_file);
  dot_file.close();

  if (cycdet()) {
    cerr << "Cycle detected. Exiting." << endl;
    return 1;
  }

  ofstream wave_file("example8.vcd");
  run(wave_file, 80);
  
  ofstream netlist_file("example8.nand");
  print_netlist(netlist_file);

  report();

  return 0;
}
