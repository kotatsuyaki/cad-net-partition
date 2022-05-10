#include "data.hpp"

#include <cstdlib>

#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <fmt/format.h>
#include <gsl/gsl>
#include <range/v3/all.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace views = ranges::views;
namespace actions = ranges::actions;
using ranges::range;
using ranges::span;
using ranges::to;

template <typename T>
T& sample(std::vector<T>& vec) {
  Expects(vec.empty() == false);
  return *(vec | views::sample(1)).begin();
}

int main(int argc, char** argv) try {
  assert(argc >= 2);
  std::ifstream infile(argv[1]);

  const InputData inputs = InputData::read_from(infile);

  fmt::print("Cell areas: {}\n", fmt::join(inputs.cell_areas, ", "));
  for (const auto& [i, net] : inputs.nets | views::enumerate) {
    fmt::print("Net {} contains cells: {}\n", i,
               fmt::join(net | to<std::vector>() | actions::sort, ", "));
  }

  // Calculate minimum number of groups needed
  const size_t ngroups = inputs.min_number_of_groups();
  fmt::print("ngroups = {}\n", ngroups);

  // Obtain starting partition
  std::vector<Group> groups(ngroups);
  assert(groups.size() == ngroups);

  for (size_t i = 0; i < inputs.ncells; i += 1) {
    // Find all the groups with the minimum area
    const size_t min_group_area = ranges::min_element(groups)->area;
    const auto has_min_area = [min_group_area](const auto& g) {
      return g.area == min_group_area;
    };
    auto groups_with_min_area =
        groups | views::filter(has_min_area) | to<std::vector<Group>>();

    // Sample one group and put the cell into it
    auto& sampled = sample(groups_with_min_area);
    sampled.cells.emplace_back(i);
    sampled.area += inputs.cell_areas[i];

    if (sampled.area > inputs.max_group_area) {
      throw std::runtime_error("Group area exceeded the limit");
    }
  }

  return 0;
} catch (const std::exception& e) {
  fmt::print(stderr, "Exception caught at main(): {}\n", e.what());
  return 1;
}
