#ifndef CHDL_PARSER_H
#define CHDL_PARSER_H

#include "node.h"

#include <string>
#include <vector>
#include <iostream>

namespace chdl {
  template <typename T, typename U>
    void parse(std::istream in, T &header_func, U &instance_func);

  struct parser_callback_base_t {
    virtual void call_header(
      const std::string &section,
      const std::string &name,
      const std::vector<nodeid_t> &nodes
    ) = 0;

    virtual void call_instance(
      const std::string &type,
      const std::vector<std::string> &params,
      const std::vector<nodeid_t> &nodes
    ) = 0;
  };

  template <typename T, typename U>
    struct parser_callback_t : public parser_callback_base_t
  {
    parser_callback_t(T &h, U &i): header_callback(h), instance_callback(i) {}

    virtual void call_header(
      const std::string &section,
      const std::string &name,
      const std::vector<nodeid_t> &nodes
    ) { header_callback(section, name, nodes); }

    virtual void call_instance(
      const std::string &type,
      const std::vector<std::string> &params,
      const std::vector<nodeid_t> &nodes
    ) { instance_callback(type, params, nodes); }

    T &header_callback;
    U &instance_callback;
  };

  void parse(std::istream in, parser_callback_base_t &cb);

  template <typename T, typename U>
    void parse(std::istream in, T &header_func, U &instance_func)
  {
    parser_callback_t<T, U> cb(header_func, instance_func);
    parse(cb);
  }
}

#endif
