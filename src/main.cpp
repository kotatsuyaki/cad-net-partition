#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <fmt/format.h>

#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

using std::unordered_set;
using std::vector;
using Net = unordered_set<size_t>;

int main(int argc, char** argv) {
  std::ifstream infile(argv[1]);

  size_t max_group_area = 0;
  size_t ncells = 0;
  size_t nnets = 0;

  std::string ignore_word;

  infile >> max_group_area;
  infile >> ignore_word;
  assert(ignore_word == ".cell");
  infile >> ncells;

  vector<size_t> areas(ncells, 0);
  for (size_t i = 0; i < ncells; i += 1) {
    size_t index = 0;
    infile >> index;
    infile >> areas[index];
  }

  fmt::print(stderr, "{}", fmt::join(areas, ", "));

  infile >> ignore_word;
  assert(ignore_word == ".net");

  infile >> nnets;
  vector<Net> nets(nnets, Net());

  // `i` is the index of net
  for (size_t i = 0; i < nnets; i += 1) {
    size_t ncells_contained = 0;
    infile >> ncells_contained;
    for (size_t j = 0; j < ncells_contained; j += 1) {
      size_t cell = 0;
      infile >> cell;
      nets[i].insert(cell);
    }
  }

  for (const auto& net : nets) {
    fmt::print(stderr, "{}", fmt::join(net, ", "));
  }

  return 0;
}
