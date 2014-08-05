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

    template <typename T>
      void push(T x_in)
    {
      unsigned long x((unsigned long)x_in);
      for (unsigned i = 0; i < sizeof(x_in); ++i) {
        push(char(x & 0xff));
        x >>= 8;
      }
    }

  private:
    void reallocate(size_t sz);

    char *buf, *pos, *end;
  };
}

#endif
