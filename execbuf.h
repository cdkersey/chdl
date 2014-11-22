#ifndef CHDL_EXECBUF_H
#define CHDL_EXECBUF_H

#include <cstdlib>

namespace chdl {
  class execbuf {
  public:
    execbuf();

    void dump();

    unsigned operator()(long c=0, long b=0, long d=0);
    void push(char x);

    void clear();

    template <typename T>
      void push(T x_in)
    {
      unsigned long x((unsigned long)x_in);
      for (unsigned i = 0; i < sizeof(x_in); ++i) {
        push(char(x & 0xff));
        x >>= 8;
      }
    }

    // Return a handle that can be used to fix up a value later
    template <typename T> int push_future() {
      int prev_pos(pos - buf);
      for (unsigned i = 0; i < sizeof(T); ++i) push(char(0));
      return prev_pos;
    }

    // Fix up a value.
    void push(int offset, char val) { buf[offset] = val; }

    template <typename T> void push(int offset, T x_in) {
      unsigned long x((unsigned long)x_in);
      for (unsigned i = 0; i < sizeof(x_in); ++i) {
        push(offset + i, char(x & 0xff));
        x >>= 8;
      }
    }

    int get_pos() { return pos - buf; }

    void reallocate(size_t sz);

    char *buf, *pos, *end;
  };
}

#endif
