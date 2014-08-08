// Tickable.
#include "tickable.h"
#include "cdomain.h"
#include "reset.h"
#include "execbuf.h"

using namespace std;
using namespace chdl;

vector<vector<tickable*> > &chdl::tickables() {
  static vector<vector<tickable*> > t{vector<tickable*>()};
  return t;
}

static void reset_tickables() {
  tickables() = vector<vector<tickable*> >{vector<tickable*>()};
}

extern "C" {
  unsigned tickable_call_pre_tick(tickable *p, evaluator_t *e);
  unsigned tickable_call_tick(tickable *p, evaluator_t *e);
  unsigned tickable_call_tock(tickable *p, evaluator_t *e);
  unsigned tickable_call_post_tock(tickable *p, evaluator_t *e);
}

void tickable::gen_pre_tick(evaluator_t &e, execbuf &b,
                            nodebuf_t &from, nodebuf_t &to)
{
  b.push(char(0x48)); // mov this, %rdi
  b.push(char(0xbf));
  b.push(this);

  b.push(char(0x48));
  b.push(char(0xbe)); // mov &e, %rsi
  b.push(&e);

  b.push(char(0x48)); // mov &tickable_call_pre_tick, %rbx
  b.push(char(0xbb));
  b.push((void*)&tickable_call_pre_tick);

  b.push(char(0xff)); // callq *%rbx
  b.push(char(0xd3));
}

void tickable::gen_tick(evaluator_t &e, execbuf &b,
                        nodebuf_t &from, nodebuf_t &to)
{
  b.push(char(0x48)); // mov this, %rdi
  b.push(char(0xbf));
  b.push(this);

  b.push(char(0x48));
  b.push(char(0xbe)); // mov &e, %rsi
  b.push(&e);

  b.push(char(0x48)); // mov &tickable_call_tick, %rbx
  b.push(char(0xbb));
  b.push((void*)&tickable_call_tick);

  b.push(char(0xff)); // callq *%rbx
  b.push(char(0xd3));
}

void tickable::gen_tock(evaluator_t &e, execbuf &b,
                        nodebuf_t &from, nodebuf_t &to)
{
  b.push(char(0x48)); // mov this, %rdi
  b.push(char(0xbf));
  b.push(this);

  b.push(char(0x48));
  b.push(char(0xbe)); // mov &e, %rsi
  b.push(&e);

  b.push(char(0x48)); // mov &tickable_call_tock, %rbx
  b.push(char(0xbb));
  b.push((void*)&tickable_call_tock);

  b.push(char(0xff)); // callq *%rbx
  b.push(char(0xd3));
}

void tickable::gen_post_tock(evaluator_t &e, execbuf &b,
                             nodebuf_t &from, nodebuf_t &to)
{
  b.push(char(0x48)); // mov this, %rdi
  b.push(char(0xbf));
  b.push(this);

  b.push(char(0x48));
  b.push(char(0xbe)); // mov &e, %rsi
  b.push(&e);

  b.push(char(0x48)); // mov &tickable_call_post_tock, %rbx
  b.push(char(0xbb));
  b.push((void*)&tickable_call_post_tock);

  b.push(char(0xff)); // callq *%rbx
  b.push(char(0xd3));
}

CHDL_REGISTER_RESET(reset_tickables);

// Destroying a tickable is O(N^2)
tickable::~tickable() {
  vector<tickable*> dest;
  for (unsigned j = 0; j < tickables().size(); ++j) {
    bool found(false);
    for (unsigned i = 0; i < tickables()[j].size(); ++i)
      if (tickables()[j][i] == this) found = true;

    if (found) {
      for (unsigned i = 0; i < tickables()[j].size(); ++i)
        if (tickables()[j][i] != this) dest.push_back(tickables()[j][i]);

      tickables()[j] = dest;
    }
  }
}

unsigned tickable_call_pre_tick(tickable *p, evaluator_t *e) {
  p->pre_tick(*e);
}

unsigned tickable_call_tick(tickable *p, evaluator_t *e) {
  p->tick(*e);
}

unsigned tickable_call_tock(tickable *p, evaluator_t *e) {
  p->tock(*e);
}

unsigned tickable_call_post_tock(tickable *p, evaluator_t *e) {
  p->post_tock(*e);
}
