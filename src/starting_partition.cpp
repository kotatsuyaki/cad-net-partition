#include "starting_partition.hpp"

#include <gsl/gsl>
#include <range/v3/all.hpp>

using ranges::to;
using ranges::views::enumerate;
using ranges::views::filter;
using ranges::views::transform;

using std::optional;
using std::pair;
using std::vector;

namespace {

// Samples a vector element at random and returns by value.
template <typename T>
T sample(const vector<T>& vec) {
  Expects(vec.empty() == false);
  return *(vec | ranges::views::sample(1)).begin();
}

// Gets first element out of an (index, group) pair.
decltype(auto) first(pair<size_t, const Group&> p) { return p.first; }

// Finds indexes of all the groups with the minimum area.
vector<size_t> indexes_with_min_area(const vector<Group>& groups) {
  // Get min area
  const size_t min_group_area = ranges::min_element(groups)->area;

  const auto has_min_area = [min_group_area](const auto& p) {
    return p.second.area == min_group_area;
  };
  return groups | enumerate | filter(has_min_area) | transform(first) |
         to<vector<size_t>>();
}

// Finds an ngroups-way starting partition.
// Returns nullopt if group area limit is violated.
optional<vector<Group>> find_starting_partition(const InputData& inputs,
                                                size_t ngroups) {
  vector<Group> groups(ngroups);

  for (size_t i = 0; i < inputs.ncells; i += 1) {
    const auto indexes = indexes_with_min_area(groups);

    // Sample one group and put the cell into it
    const size_t gi = sample(indexes);
    auto& sampled = groups[gi];

    sampled.cells.push_back(i);
    sampled.area += inputs.cell_areas[i];

    // Return nullopt upon group area violation
    if (sampled.area > inputs.max_group_area) {
      return std::nullopt;
    }
  }

  return groups;
}

}  // namespace

// Finds an starting partition with unspecified (but legal) #groups.
// Throws if it is impossible to partition.
vector<Group> find_starting_partition(const InputData& inputs) noexcept(false) {
  // TODO: Find better strategy than "k += 1"
  for (size_t k = inputs.min_number_of_groups(); k <= inputs.ncells; k += 1) {
    if (auto groups = find_starting_partition(inputs, k)) {
      return *groups;
    }
    fmt::print("Failed to find starting partition with k = {}\n", k);
  }
  throw std::runtime_error("Failed to find starting partition");
}
