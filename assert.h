// The assertion system allows certain error conditions to cause simulation to
// finish with an error message
#ifndef CHDL_ASSERT_H
#define CHDL_ASSERT_H
#include <string>

#include "node.h"

namespace chdl {
  void assert_node_true(std::string text, node node);
};

#define CHDL_STRINGIFY_INTERNAL(x) #x
#define CHDL_STRINGIFY(x) CHDL_STRINGIFY_INTERNAL(x)
#define CHDL_LINE_STR CHDL_STRINGIFY(__LINE__)

#ifndef CHDL_DISABLE_ASSERT
#ifdef ASSERT
  // ASSERT has already been defined by another library. This is only a problem
  // if it was used in CHDL code.
  #error ASSERT already #defined. #undef it or define CHDL_DISABLE_ASSERT.
#else
#define ASSERT(x) do { \
    assert_node_true(std::string("Failed assertion in ") + \
                     __FILE__  ": Line " CHDL_LINE_STR, x); \
} while(0)
#endif
#endif

#endif
