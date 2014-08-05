#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>

extern "C" {
  #include <sys/mman.h>
}

#include "execbuf.h"

using namespace std;

execbuf::execbuf():
  buf(0), pos(0), end(buf + 1)
{}

void execbuf::dump() {
  unsigned col(0);
  for (char *p = buf; p != pos; ++p) {
    cout << hex << setw(2) << setfill('0') << unsigned(*p & 0xff);

    col++;
    if (col != 0) cout << ' ';
    if (col == 8) cout << ' ';
    if (col == 16) { cout << '\n'; col = 0; }
  }
  if (col) cout << endl;
}

unsigned execbuf::operator()(long c, long b, long d)
{
  cout << "Calling buf at " << (void*)buf << endl;

  unsigned val;
  __asm__ __volatile__("call *%%rax" :
                       "=a"(val),"=c"(c),"=b"(b),"=d"(d) : 
                        "a"(buf), "c"(c), "b"(b), "d"(d) );

  cout << "returned " << val << ", " << c << ", " << b << ", " << d << endl;

  return val;
}

void execbuf::push(char x) {
  if (pos == end - 1)
    reallocate((end - buf)*2);

  *(pos++) = x;  
}

void execbuf::reallocate(size_t sz) {
  cout << "Reallocate execbuf: " << dec << sz << endl;

  size_t old_sz(end - buf - 1);
  char *old_buf(buf);

  buf = (char*)mmap(NULL, sz, PROT_EXEC | PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

  end = buf + sz;
  pos = pos - old_buf + buf;

  memcpy(buf, old_buf, old_sz);

  munmap(old_buf, old_sz);
}
