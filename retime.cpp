#include "analysis.h"
#include "opt.h"
#include "tap.h"
#include "input.h"
#include "gates.h"
#include "nodeimpl.h"
#include "gatesimpl.h"
#include "litimpl.h"
#include "netlist.h"
#include "regimpl.h"
#include "lit.h"
#include "node.h"
#include "memory.h"
#include "submodule.h"
#include "trisimpl.h"
#include "tristate.h"

#include <fstream>
#include <random>
#include <vector>
#include <set>
#include <queue>
#include <list>
#include <map>

#define DEBUG

using namespace chdl;
using namespace std;

const double RCOUNT_MULT = 1;

struct node_edge {
  node_edge(int w, int s, int d, nodeid_t n = 0):
    weight(w), src(s), dest(d), output_id(n) {}

  int weight, src, dest;
  nodeid_t output_id; // nodeid of output
};

ostream &operator<<(ostream &out, const node_edge &e) {
  out << e.src << " -" << e.weight << "-> " << e.dest;
  if (e.dest == -1) out << '(' << e.output_id << ')';

  return out;
}

struct node_meta {
  int path; // Length of path (0-dflop edges) to this node
  vector<int> in, out;
  bool is_reg;
  int relpos, rcount;
  vector<bool> initvals;
};

struct retimer_t {
  retimer_t(int iters, int moves);
  ~retimer_t();

  void retime();

  void retime_step(double temp);
  void update_path(nodeid_t i, bool fwd);
  void update_pl();
  void compute_initvals();
  void compute_score();
  
  void print_graph();

  double score;
  default_random_engine rng;
  uniform_real_distribution<double> dkeep;
  map<int, set<int> > pl;
  vector<node_meta> m;
  vector<node_edge> e;
  unsigned iters, moves;
};

retimer_t::retimer_t(int iters, int moves):
  iters(iters), moves(moves), m(nodes.size()), dkeep(0.0, 1.0)
{
  // 1. Construct the metadata structure.
  for (unsigned i = 0; i < nodes.size(); ++i) {
    m[i].is_reg = false;
    m[i].path = 0;
    m[i].relpos = 0;
    
    if (regimpl *p = dynamic_cast<regimpl*>(nodes[i])) {
      m[i].is_reg = true;
      m[nodeid_t(p->d)].out.push_back(e.size());
      m[i].in.push_back(e.size());
      e.push_back(node_edge(0, nodeid_t(p->d), i));
    }

    for (auto &n : nodes[i]->src) {
      m[nodeid_t(n)].out.push_back(e.size());
      m[i].in.push_back(e.size());
      e.push_back(node_edge(0, nodeid_t(n), i));
    }
  }

  // Find all of the taps/ghost taps. These are edges to -1
  set<nodeid_t> taps;
  get_tap_nodes(taps);
  for (auto &t : taps) {
    m[t].out.push_back(e.size());
    e.push_back(node_edge(0, t, -1, t));
  }

  // 2. Compute initial path lengths
  queue<int> pl_q;
  taps.clear();
  get_input_nodes(taps);
  for (auto &t : taps)
    pl_q.push(t);
  for (unsigned i = 0; i < nodes.size(); ++i)
    if (regimpl *p = dynamic_cast<regimpl*>(nodes[i]))
      pl_q.push(i);

  while (!pl_q.empty()) {
    int nidx = pl_q.front();
    pl_q.pop();

    if (nidx == -1) continue;
    
    int new_pl = m[nidx].path + 1;
    for (auto &eidx : m[nidx].out) {
      int d = e[eidx].dest;
      if (!m[d].is_reg && new_pl > m[d].path) {
	m[d].path = new_pl;
	pl_q.push(d);
      }
    }
  }

  // 3. Replace regs with edge weights
  bool changed;
  do {
    changed = false;
    // Find each DFF with no DFF successors and skip it.
    for (unsigned i = 0; i < nodes.size(); ++i) {
      if (!m[i].is_reg) continue;

      // Do a BFS to determine if this node is reachable by itself
      // passing only through DFFs. Cycles of DFFs are a nonsensical
      // structure. If one is found, display an error message and exit.
      { queue<int> bfs_q;
        set<int> visited;
        bfs_q.push(i);
        while (!bfs_q.empty()) {
	  int x = bfs_q.front();
	  bfs_q.pop();
          visited.insert(x);
          for (auto &idx : m[i].out) {
            int d = e[idx].dest;
            if (d == i) {
              cout << "Register retiming error: Reg cycle" << endl;
              exit(1);
            }
            if (visited.count(d)) continue;
            bfs_q.push(d);
          }
        }
      }

      changed = true;
      
      bool skip = false;
      for (unsigned j = 0; j < m[i].out.size(); ++j) {
	if (e[m[i].out[j]].dest == -1) continue;
	if (m[e[m[i].out[j]].dest].is_reg) {
	  skip = true;
	  break;
	}
      }
      if (skip) continue;

      // Bypass this node and add 1 to weights through it.
      if (m[i].in.size() != 1) {
	cout << "Register retiming error: Reg with other than 1 pred" << endl;
	exit(1);
      }

      int prev = e[m[i].in[0]].src;      
      for (unsigned j = 0; j < m[prev].out.size(); ++j)
	if (e[m[prev].out[j]].dest == i)
	  m[prev].out.erase(m[prev].out.begin() + j);
  
      for (unsigned j = 0; j < m[i].out.size(); ++j) {
        int idx = m[i].out[j];

	e[idx].src = prev;
	e[idx].weight++;
	m[prev].out.push_back(idx);
      }

      // Delete this node's metadata
      m[i].is_reg = false;
      m[i].in.clear();
      m[i].out.clear();
    }
  } while(changed);

  // 4. Compute initial reg counts
  for (unsigned i = 0; i < nodes.size(); ++i) {
    int n_regs = 0;
    for (auto &eidx : m[i].out)
      if (e[eidx].weight > n_regs)
        n_regs = e[eidx].weight;
    m[i].rcount = n_regs;
  }
 
}

retimer_t::~retimer_t() {
  // Compute the initial values of registers in the chains through simulation
  compute_initvals();
  
  // 6. Re-generate flip-flops
  for (unsigned i = 0; i < m.size(); ++i) {
    int n_regs = m[i].rcount;

    if (n_regs == 0) continue;

    vector<node> regs;
    nodeid_t id = i;
    for (unsigned j = 0; j < n_regs; ++j) {
      node x(id);
      bool ival(false);
      if (j < -m[i].relpos) {
        ival = m[i].initvals[-m[i].relpos - j - 1];
      }
      regs.push_back(Reg(x, ival));
      id = nodeid_t(*regs.rbegin());
    }

    for (unsigned j = 0; j < m[i].out.size(); ++j) {
      int eidx = m[i].out[j], idx = 0;

      int r_idx = e[eidx].weight - 1;

      nodeid_t newnode(r_idx < 0 ? i : nodeid_t(regs[r_idx]));
 
      if (e[eidx].dest == -1) {
        change_tap(e[eidx].output_id, newnode);
	continue;
      }

      for (unsigned k = 0; k < m[e[eidx].dest].in.size(); ++k) {
	if (m[e[eidx].dest].in[k] == eidx) {
          nodeid_t d = e[eidx].dest;
          nodes[d]->src[k].change_net(newnode);
	}
      }
    }
  }

  // 7. Remove the old regs.
  for (unsigned i = 0; i < m.size(); ++i) {
    if (m[i].out.size() == 0) {
      nodeimpl *p = nodes[i];
      nodes[i] = new litimpl(0);
      nodes.pop_back();
      nodes[i]->id = i;
      nodes[i]->path = p->path;
      //delete p; // TODO : we probably shouldn't leak memory here
    }
  }
  //opt_dead_node_elimination();
}

void retimer_t::compute_score() {
  cout << "Before: " << score << endl;

  score = 0;
  for (unsigned i = 0; i < nodes.size(); ++i)
    score += m[i].rcount * RCOUNT_MULT;
  cout << "Compute score:" << endl;
  for (auto &x : pl) {
    cout << " {";
    for (auto &i : x.second) cout << ' ' << i << '(' << m[i].path << ')';
    cout << '}';
    score += x.second.size()*x.first;
  }
  cout << endl;
  cout << "After: " << score << endl;
}

void retimer_t::update_pl() {
  // Create path length map so we can identify each gate with path length X
  pl.clear();
  for (unsigned i = 0; i < nodes.size(); ++i)
    pl[m[i].path].insert(i);
}

void retimer_t::retime_step(double temp) {
  for (unsigned move = 0; move < moves; ++move) {
    nodeid_t i = uniform_int_distribution<nodeid_t>(0, nodes.size()-1)(rng);

    // Find number of spaces fwd/rev node can be moved
    int fwd = -m[i].relpos, rev = 0x7fffffff;
    if (fwd < 0) fwd = 0;
    for (auto &eidx : m[i].out)
      if (e[eidx].weight < fwd)
        fwd = e[eidx].weight;

    if (m[i].out.size() == 0) fwd = 0;
      
    for (auto &eidx : m[i].in)
      if (e[eidx].weight < rev)
        rev = e[eidx].weight;

    if (m[i].in.size() == 0) rev = 0;

    if (fwd == 0 && rev == 0) continue;

    // 1. Choose a direction
    bool move_forward;
    if (fwd && !rev)
      move_forward = true;
    else if (rev && !fwd)
      move_forward = false;
    else
      move_forward = bernoulli_distribution(0.5)(rng);

    //cout << "Trying move " << i << (move_forward ? " fwd" : " back") << endl;
    //print_graph();
    //cout << endl << endl;
    
    // 2. Perform move
    double score_before = score;

    set<int> efwd, erev;
    for (auto &eidx : m[i].out)
      efwd.insert(eidx);

    for (auto &eidx : m[i].in)
      erev.insert(eidx);

    if (move_forward) {
      ++m[i].relpos;
      for (auto &eidx : efwd)
	--e[eidx].weight;
      for (auto &eidx : erev)
	++e[eidx].weight;
    } else {
      --m[i].relpos;
      for (auto &eidx : efwd)
	++e[eidx].weight;
      for (auto &eidx : erev)
	--e[eidx].weight;
    }

    //cout << "Update path, score = " << score << endl;
    update_path(i, move_forward);
    double delta_score = score - score_before;

    // 3. Decide whether to keep the move
    double pkeep = (delta_score < 0) ? 1.0 : exp(-delta_score/temp);

    if (dkeep(rng) < pkeep) {
      continue;
    }

    // 4. Reverse the move if we decided not to keep it
    if (!move_forward) {
      ++m[i].relpos;
      for (auto &eidx : efwd)
	--e[eidx].weight;
      for (auto &eidx : erev)
	++e[eidx].weight;
    } else {
      --m[i].relpos;
      for (auto &eidx : efwd)
	++e[eidx].weight;
      for (auto &eidx : erev)
	--e[eidx].weight;
    }

    //cout << "Undoing move " << i << (move_forward ? " fwd" : " back") << "..." << endl;
    update_path(i, !move_forward);
    score = score_before;
  }
}

void retimer_t::retime() {
  cout << "Before retiming:" << endl;
  update_pl();  
  for (auto &s : pl)
    cout << ' ' << s.second.size();
  cout << endl;
  compute_score();
  cout << "Score: " << score << endl;
  
  // 5. Perform simulated annealing optimization
  for (unsigned iter = 0; iter < iters; ++iter) {
    double temp((iters - iter - 1)/double(iters)*100.0);
    retime_step(temp);
    cout << iter << '(' << temp << "): " << score << endl;
    compute_score();
    cout << iter << '(' << temp << "): " << score << endl;
  }

  cout << "After retiming:" << endl;
  for (auto &s : pl)
    if (s.second.size() > 0)
      cout << ' ' << s.second.size();
  cout << endl;
  cout << "Score: " << score << endl;
}

void retimer_t::compute_initvals() {
  int sim_cyc = 0;
  for (unsigned i = 0; i < nodes.size(); ++i) {
    if (m[i].is_reg || m[i].relpos == 0) continue;
    if (-m[i].relpos > sim_cyc)
      sim_cyc = -m[i].relpos;
  }

  for (unsigned cyc = 0; cyc < sim_cyc; ++cyc) {
    for (unsigned i = 0; i < nodes.size(); ++i) {
      if (m[i].is_reg || m[i].relpos == 0) continue;
      if (m[i].relpos > 0) {
        cout << "Internal error: retiming should only move flip-flops forward."
             << endl;
        exit(1);
      }

      int n = -m[i].relpos;

      // TODO: other cdomains
      m[i].initvals.push_back(nodes[i]->eval(0));
    }

    advance();
  }
  
  reset_sim();
}

void retimer_t::update_path(nodeid_t i, bool fwd) {
  //cout << " === " << i << " === " << endl;
  //compute_score();
  //cout << "Initial score: " << score << endl;

  queue<nodeid_t> q;
  q.push(i);
  for (auto &eidx : m[i].in)
    q.push(e[eidx].src);
  for (auto &eidx : m[i].out)
    if (e[eidx].dest != -1)
      q.push(e[eidx].dest);

  while (!q.empty()) {
    nodeid_t idx = q.front();
    q.pop();

    // Update reg count
    int n_regs = 0;
    for (auto &eidx : m[idx].out)
      if (e[eidx].weight > n_regs)
        n_regs = e[eidx].weight;
    score += (n_regs - m[idx].rcount)*RCOUNT_MULT;
    //cout << idx << "(reg): -" << m[idx].rcount << " / +" << n_regs << endl; 
    m[idx].rcount = n_regs;
    
    int path = 1;
    //cout << "Incoming:";
    for (auto &eidx : m[idx].in) {
      if (e[eidx].weight > 0) continue;
      //cout << ' ' << e[eidx].src << '(' << m[e[eidx].src].path << ')';
      int new_path = m[e[eidx].src].path + 1;
      if (new_path > path) path = new_path;
    }
    //cout << endl;

    if (m[idx].path != path) {
      for (auto &eidx : m[idx].out) {
        if (e[eidx].dest != -1) {
          q.push(e[eidx].dest);
        }
      }
      
      pl[m[idx].path].erase(idx);
      pl[path].insert(idx);
      //cout << idx << ": -" << m[idx].path << " / +" << path << endl;
      score -= m[idx].path;
      score += path;
      m[idx].path = path;
    }
  }

  //double old_score = score;
  //compute_score();
  //cout << "upath: " << old_score << " vs " << score << endl;
  //if (abs(old_score - score) > 1e-3) cout << "MISMATCH on node " << i << endl;
}

void retimer_t::print_graph() {
  // 4. Print the initial graph
  #ifdef DEBUG

  cout << " \\/ \\/ \\/ \\/ pl:";
  for (auto &x : pl) {
    cout << " {";
    for (auto &i : x.second) cout << ' ' << i << '(' << m[i].path << ')';
    cout << '}';
  }
  cout << endl;
  
  for (unsigned i = 0; i < m.size(); ++i) {
    cout << "Node " << i << " <" << m[i].path << '>';
    if (m[i].is_reg) cout << " (flip-flop)";
    if (m[i].relpos) cout << " " << m[i].relpos;
    cout << endl;
    cout << "  out:" << endl;
    for (auto idx : m[i].out)
      cout << "    " << e[idx] << endl;
    cout << "  in:" << endl;
    for (auto idx : m[i].in)
      cout << "    " << e[idx] << endl;
  }
  #endif
}

void chdl::opt_reg_retime(int iters) {
  unsigned moves(nodes.size() * 1000);

  retimer_t r(iters, moves);
  r.retime();
  //r.print_graph();
}
