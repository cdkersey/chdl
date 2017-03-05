#include <sstream>
#include <string>
#include <iomanip>

#include "parser.h"

using namespace chdl;
using namespace std;

const unsigned MAX_LINE = 1024;

string get_instance_type(std::istream &in);

void chdl::parse(std::istream &in, parser_callback_base_t &cb) {
  char line_buffer[MAX_LINE];
  bool in_section = false, in_design = false;
  string section;

  while (!!in) {
    in.getline(line_buffer, MAX_LINE);
    if (!in) break;

    istringstream line((string(line_buffer)));
    line >> skipws;

    if (line.peek() == ' ') {
      if (!in_section) {
        istringstream line(line_buffer);
        // TODO: this is an error, no instances/header outside of sections.
      }

      if (!in_design) {
        // Parse as header line
        string name;
        vector<nodeid_t> v;

        line >> name;

        while (!!line) {
          nodeid_t id;
          line >> id;
          if (!!line) v.push_back(id);
        }
        
        cb.call_header(section, name, v);
      } else {
        // Parse as instance line
        string type;
        vector<string> params;
        vector<nodeid_t> v;

        type = get_instance_type(line);

        while (line.peek() == ' ' || line.peek() == '\t') line.get();

        if (line.peek() == '<') {
          line.get();

          char c;
          bool string_mode = false, delim_mode = false;
          params.push_back(string());
          while(!!line && (c = line.get()) != '>') {
            if (!string_mode && c == ' ') params.push_back(string());
            else if (!delim_mode && c == '\"') string_mode = !string_mode;
            else params.rbegin()->push_back(c);

            if (string_mode && !delim_mode && c == '\\') delim_mode = true;
            else delim_mode = false;
	  }
	}

        while (!!line) {
          nodeid_t id;
          line >> id;
          if (!!line) v.push_back(id);
        }

        cb.call_instance(type, params, v);
      }
    } else if (line.peek() != '#') {
      in_section = true;
      section = line_buffer;
      if (section == "design") in_design = true;
    }
  }
}

string get_instance_type(std::istream &in) {
  char c;
  string type;

  while (!!in && in.peek() == ' ' || in.peek() == '\t') in.get(); 

  c = in.peek();
  while (!!in && c != '\n' && c != ' ' && c != '<') {
    c = in.get();
    if (!!in) type.push_back(c);
    c = in.peek();
  }

  return type;
}
