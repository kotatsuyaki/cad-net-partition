#include "cost.hpp"
#include "data.hpp"
#include "starting_partition.hpp"

#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <fmt/format.h>
#include <gsl/gsl>
#include <range/v3/all.hpp>

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <stdexcept>

using ranges::views::enumerate;
using std::ifstream;
using std::string;
using std::vector;

int main(int argc, char** argv) try {
  assert(argc >= 2);
  ifstream infile(argv[1]);  // NOLINT

  const InputData inputs = InputData::read_from(infile);

  inputs.debug_print();

  const vector<Group> groups = find_starting_partition(inputs);

  for (const auto& [i, group] : groups | enumerate) {
    fmt::print("Group {} (area = {}) contains cells: {}\n", i, group.area,
               fmt::join(group.cells, ", "));
  }

  fmt::print("Cost of starting partition = {}\n", find_cost(groups, inputs));

  return 0;
} catch (const std::exception& e) {
  fmt::print(stderr, "Exception caught at main(): {}\n", e.what());
  return 1;
}
