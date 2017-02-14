#include <iostream>
#include <queue>
#include <thread>
#include <mutex>

#include "console.h"
#include "ingress.h"
#include "egress.h"
#include "tickable.h"

using namespace chdl;
using namespace std;

template <typename T, unsigned N>
  void sim_val(T &o, const bvec<N> &x, cdomain_handle_t cd = 0)
{
  unsigned i;

  for (i = 0, o = 0; i < N; ++i) {
    o <<= 1;
    o |= (nodes[nodeid_t(x[N - i - 1])]->eval(cd) ? 1 : 0);
  }
}

void chdl::ConsoleOut(node valid, bvec<8> x, std::ostream &out) {
  gtap(x);

  cdomain_handle_t cd(cur_clock_domain());

  EgressFunc([x, cd, &out](bool v) {
    if (v) {
      char c;
      sim_val(c, x, cd);
      out << c;
    }
  }, valid);
}

struct console_t : public tickable {
  console_t(node v, bvec<8> x, istream &in): c(0), valid(false), in(in)
  {
    x = IngressInt<8>(c);
    v = Ingress(valid);
    t = new thread([this, &in]{
      char c;
      while (in.get(c)) {
        m.lock(); {
          q.push(c);
        } m.unlock();
      }
    });
  }

  void tick(cdomain_handle_t cd) {
    m.lock(); {
      valid = !q.empty();
      if (valid) {
        c = q.front();
        q.pop();
      } else {
        c = 0;
      }
    } m.unlock();
  }

  char c;
  bool valid;
  queue<char> q;
  mutex m;
  thread *t;
  
  std::istream &in;

  static vector<console_t*> console_ts;
};

vector<console_t*> console_t::console_ts;

void chdl::ConsoleIn(const node &valid, const bvec<8> &x, std::istream &in) {
  console_t *c = new console_t(valid, x, in);
}
