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
    template <typename T> char *push_future() {
      char *prev_pos(pos);
      for (unsigned i = 0; i < sizeof(T); ++i) push(char(0));
      return prev_pos;
    }

    // Fix up a value.
    void push(char *ptr, char val) { *ptr = val; }

    template <typename T> void push(char *ptr, T x_in) {
      unsigned long x((unsigned long)x_in);
      for (unsigned i = 0; i < sizeof(x_in); ++i) {
        push(ptr + i, char(x & 0xff));
        x >>= 8;
      }
    }

  private:
    void reallocate(size_t sz);

    char *buf, *pos, *end;
  };
}

#endif
