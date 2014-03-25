// The assertion system allows certain error conditions to cause simulation to
// finish with an error message
#ifndef __ASSERT_H
#define __ASSERT_H
#include <string>

#include "node.h"

namespace chdl {
  void assert(std::string text, node node);
};

#define CHDL_STRINGIFY_INTERNAL(x) #x
#define CHDL_STRINGIFY(x) CHDL_STRINGIFY_INTERNAL(x)
#define CHDL_LINE_STR CHDL_STRINGIFY(__LINE__)

#define ASSERT(x) do { \
    assert(std::string("Failed assertion in ") + \
           __FILE__  ": Line " CHDL_LINE_STR, x); \
} while(0)

#endif
