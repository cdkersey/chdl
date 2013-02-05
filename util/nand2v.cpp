// This is hopefully the start of a tool that will enable synthesis and faster
// simulation, starting with a "nand file" style netlist.
#include <iostream>
#include <iomanip>
#include <map>
#include <vector>
#include <set>
#include <cctype>
#include <algorithm>

using namespace std;

typedef unsigned long node_t;

map<string, vector<node_t> > inputs;
map<string, vector<node_t> > outputs;

enum node_type {
  LIT0_NODE, LIT1_NODE, INV_NODE, NAND_NODE, RAMQ_NODE, REGQ_NODE
};

// Type lookup map and its reverse, so we can get node->type and type->nodes
map<node_t, node_type> type;
typedef map<node_t, node_type>::iterator type_it;
map<node_type, set<node_t> > rtype;
node_t maxnode(0);

// Vector representation of the design.
typedef pair<node_t, node_t> node_pair;
vector<node_pair> nand_in;
vector<node_t> inv_in, inv_out, nand_out, reg_q, reg_d, lit0, lit1, ram_w;

vector<vector<node_t> > ram_qa, ram_q, ram_da, ram_d;
vector<string> ram_file;
map<node_t, unsigned> ram_map; // Get ram index given node.

// Maps levels to the nodes that appear at each.
map<unsigned, set<node_t> > depthmap;

// Dependency map:
map<node_t, set<node_t> > depmap;
typedef map<node_t, set<node_t> >::iterator depmap_it_t;
typedef set<node_t>::iterator ns_it_t;

void getnums(vector<node_t> &v, istream &infile) {
  for (;;) {
    node_t num;
    while (infile.peek() == ' ') infile.get();
    if (infile.peek() == '\n') { infile.get(); return; }
    infile >> num;
    v.push_back(num);
  }
}

bool read_nandfile(istream &infile) {
  string s;

  // Read the inputs.
  infile >> s;
  if (s != "inputs") {
    cerr << "Malformed input." << endl;
    return false;
  }
  while (infile.get() != '\n');
  while (infile.peek() == ' ') {
    string iname;
    infile >> iname;
    getnums(inputs[iname], infile);
  }

  // Read the outputs.
  infile >> s;
  if (s != "outputs") {
    cerr << "Malformed input." << endl;
    return false;
  }

  while (infile.get() != '\n');
  while (infile.peek() == ' ') {
    string oname;
    infile >> oname;
    getnums(outputs[oname], infile);
  }

  // Read the gates.
  infile >> s;
  if (s != "design") {
    cerr << "Malformed input. Expecting \"design\"." << endl;
    return false;
  }

  while (infile.get() != '\n');
  while (infile.peek() == ' ') {
    vector<node_t> v;
    string lname;
    infile >> lname;

    if (lname == "reg") {
      getnums(v, infile);
      reg_d.push_back(v[0]);
      reg_q.push_back(v[1]);
      type[v[1]] = REGQ_NODE;
    } else if (lname == "lit0") {
      getnums(v, infile);
      lit0.push_back(v[0]);
      type[v[0]] = LIT0_NODE;
    } else if (lname == "lit1") {
      getnums(v, infile);
      lit1.push_back(v[0]);
      type[v[1]] = LIT1_NODE;
    } else if (lname == "inv") {
      getnums(v, infile);
      inv_in.push_back(v[0]);
      inv_out.push_back(v[1]);
      type[v[1]] = INV_NODE;
      depmap[v[1]].insert(v[0]);
    } else if (lname == "nand") {
      getnums(v, infile);
      nand_in.push_back(node_pair(v[0], v[1]));
      nand_out.push_back(v[2]);
      type[v[2]] = NAND_NODE;
      depmap[v[2]].insert(v[0]);
      depmap[v[2]].insert(v[1]);
    } else if (lname == "syncram") {
      char junk;
      unsigned m, n;
      infile >> ws >> junk >> m >> n >> ws >> s;
      getnums(v, infile);

      string file(s.length() <= 3 ? string("") : s.substr(1, s.length() - 3));
      ram_file.push_back(file);

      ram_da.push_back(vector<node_t>());
      ram_d.push_back(vector<node_t>());
      ram_qa.push_back(vector<node_t>());
      ram_q.push_back(vector<node_t>());
      vector<node_t> &da(ram_da[ram_da.size()-1]), &d(ram_d[ram_d.size()-1]),
                     &qa(ram_qa[ram_qa.size()-1]), &q(ram_q[ram_q.size()-1]);

      for (unsigned i = 0; i < m; ++i) da.push_back(v[i]);
      for (unsigned i = 0; i < n; ++i) d.push_back(v[m+i]);
      ram_w.push_back(v[m+n]);
      for (unsigned i = 0; i < m; ++i) qa.push_back(v[m+n+1+i]);
      for (unsigned i = 0; i < n; ++i) {
        type[v[2*m+n+1+i]] = RAMQ_NODE;
        q.push_back(v[2*m+n+1+i]);
        ram_map[v[2*m+n+1+i]] = ram_qa.size() - 1;
      }

      for (unsigned i = 0; i < n; ++i) {
        for (unsigned j = 0; j < m; ++j) {
          depmap[q[i]].insert(qa[j]);
          depmap[q[i]].insert(da[j]);
        }
      }
    } else {
      cerr << "Unsupported node type \"" << lname << "\"." << endl;
      return false;
    }
  }

  // Populate the reverse node type lookup map.
  for (type_it it = type.begin(); it != type.end(); ++it) {
    rtype[it->second].insert(it->first);
  }

  return true;
}

// Write verilog module
void write_v(ostream &out) {
  // Module header 
  out << "module chdl_design(phi";
  for (auto it = inputs.begin(); it != inputs.end(); ++it)
    out << ',' << endl << "                   " << it->first;
  for (auto it = outputs.begin(); it != outputs.end(); ++it)
    out << ',' << endl << "                   " << it->first;
  out << ");" << endl
      << "  input phi;" << endl;
  for (auto it = inputs.begin(); it != inputs.end(); ++it) {
    out << "  input ";
    if (it->second.size() > 1)out << '[' << it->second.size()-1 << ":0] ";
    out << it->first << ';' << endl;
  }
  for (auto it = outputs.begin(); it != outputs.end(); ++it) {
    out << "  output ";
    if (it->second.size() > 1) out << '[' << it->second.size()-1 << ":0] ";
    out << it->first << ';' << endl;
  }
  out << endl;

  // Declare RAM arrays
  for (size_t i = 0; i < ram_qa.size(); ++i) {
    out << "  reg [" << ram_q[i].size()-1 << ":0] ram_array" << i << '['
        << (1<<ram_qa[i].size())-1 << ":0];" << endl;
  }

  // Declare register outputs and other nodes.
  for (size_t i = 0; i < reg_q.size(); i += 8) {
    out << "  reg x" << reg_q[i];
    for (size_t j = i+1; j < reg_q.size() && j < i + 8; ++j)
      out << ", x" << reg_q[j];
    out << ';' << endl;
  }
  out << endl;

  for (auto it = inputs.begin(); it != inputs.end(); ++it) {
    for (unsigned i = 0; i < it->second.size(); ++i) {
      out << "  wire x" << it->second[i] << ';' << endl;
    }
  }
  out << endl;

  for (size_t i = 0; i < ram_qa.size(); ++i) {
    out << "  wire ram_w" << i << ';' << endl;
    out << "  wire [" << ram_qa[i].size()-1 << ":0] ram_qa" << i << ';' << endl;
    out << "  reg  [" <<  ram_q[i].size()-1 << ":0] ram_q"  << i << ';' << endl;
    out << "  wire [" << ram_da[i].size()-1 << ":0] ram_da" << i << ';' << endl;
    out << "  wire [" <<  ram_d[i].size()-1 << ":0] ram_d"  << i << ';' << endl;
  }
  out << endl;

  for (size_t i = 0; i < nand_out.size(); i += 8) {
    out << "  wire x" << nand_out[i];
    for (size_t j = i+1; j < nand_out.size() && j < i + 8; ++j)
      out << ", x" << nand_out[j];
    out << ';' << endl;
  }
  out << endl;

  for (size_t i = 0; i < inv_out.size(); i += 8) {
    out << "  wire x" << inv_out[i];
    for (size_t j = i+1; j < inv_out.size() && j < i + 8; ++j)
      out << ", x" << inv_out[j];
    out << ';' << endl;
  }
  out << endl; 

  // Generate logic using static assignments
  for (size_t i = 0; i < nand_out.size(); ++i) {
    out << "  nand n" << nand_out[i] << "(x" << nand_out[i] << ", "
        << 'x' << nand_in[i].first << ", x" << nand_in[i].second
        << ");" << endl; 
  }
  out << endl;

  for (size_t i = 0; i < inv_out.size(); ++i) {
    out << "  not i" << inv_out[i] << "(x" << inv_out[i]
        << ", x" << inv_in[i] << ");" << endl;
  }
  out << endl;

  // Assign the inputs.
  for (auto it = inputs.begin(); it != inputs.end(); ++it)
    for (unsigned i = 0; i < it->second.size(); ++i) {
      out << "  assign x" << it->second[i] << " = "
          << it->first;
      if (it->second.size() > 1) out << '[' << i << ']';
      out << ';' << endl;
    }
  out << endl;

  // Assign the outputs.
  for (auto it = outputs.begin(); it != outputs.end(); ++it) {
    for (unsigned i = 0; i < it->second.size(); ++i) {
      out << "  assign " << it->first;
      if (it->second.size() > 1) out << '[' << i << ']';
      out << " = x" << it->second[i] << ';' << endl;
    }
  }
  out << endl;

  // Assign the literal 0 and 1 registers.
  for (size_t i = 0; i < lit0.size(); ++i)
    out << "  assign x" << lit0[i] << " = 0;" << endl;
  for (size_t i = 0; i < lit1.size(); ++i)
    out << "  assign x" << lit1[i] << " = 1;" << endl;
  out << endl;

  // Assign the memory inputs/outputs/address/write lines
  for (size_t i = 0; i < ram_qa.size(); ++i) {
    out << "  assign " << "ram_w" << i << " = x" << ram_w[i] << ';' << endl;
    for (size_t j = 0; j < ram_qa[i].size(); ++j) {
      out << "  assign " << "ram_qa" << i << '[' << j << "] = x" << ram_qa[i][j]
          << ';' << endl;
      out << "  assign " << "ram_da" << i << '[' << j << "] = x" << ram_da[i][j]
          << ';' << endl;
    }
    for (size_t j = 0; j < ram_d[i].size(); ++j) {
      out << "  assign " << "ram_d" << i << '[' << j << "] = x" << ram_d[i][j]
          << ';' << endl;
    }
    for (size_t j = 0; j < ram_q[i].size(); ++j) {
      out << "  assign " << 'x' << ram_q[i][j] << " = ram_q" << i << '[' << j
          << "];" << endl;
    }
  }
  out << endl;

  // Initialize registers to zero
  out << "  initial" << endl
      << "    begin" << endl;
  for (size_t i = 0; i < reg_q.size(); ++i)
    out << "      x" << reg_q[i] << " <= 0;" << endl;
  out << "    end" << endl << endl;

  // Advance registers on clock
  out << "  always @ (posedge phi)" << endl
      << "    begin" << endl;
  for (size_t i = 0; i < reg_q.size(); ++i)
    out << "       x" << reg_q[i] << " <= x" << reg_d[i] << ';' << endl;
  out << endl;
  for (size_t i = 0; i < ram_qa.size(); ++i) {
    out << "       ram_q" << i << " <= ram_array" << i << "[ram_qa" << i
        << "];" << endl;
    out << "       if (ram_w" << i << ") ram_array" << i
        << "[ram_da" << i << "] <= ram_d" << i << ';' << endl;
  }
  out << "    end" << endl;

  out << "endmodule" << endl;
}

int main(int argc, char** argv) {
  if (argc != 1) {
    cerr << "Usage:" << endl << "  "
         << argv[0] << " < input_file > output_file" << endl;
  }

  if (!read_nandfile(cin)) return 1;

  write_v(cout);

  return 0;
}
