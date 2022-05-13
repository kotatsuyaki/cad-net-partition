#include "starting_partition.hpp"

#include <gsl/gsl>
#include <range/v3/all.hpp>

#include <algorithm>

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

// Gets first element out of an (index, block) pair.
decltype(auto) first(pair<size_t, const Block&> p) { return p.first; }

// Finds indexes of all the blocks with the minimum area.
vector<size_t> indexes_with_min_area(const vector<Block>& blocks) {
  // Get min area
  const size_t min_block_area = ranges::min_element(blocks)->area;

  const auto has_min_area = [min_block_area](const auto& p) {
    return p.second.area == min_block_area;
  };
  return blocks | enumerate | filter(has_min_area) | transform(first) |
         to<vector<size_t>>();
}

// Finds an nblocks-way starting partition.
// Returns nullopt if block area limit is violated.
optional<vector<Block>> find_starting_partition(const InputData& inputs,
                                                size_t nblocks) {
  vector<Block> blocks(nblocks);

  for (size_t i = 0; i < inputs.ncells; i += 1) {
    const auto indexes = indexes_with_min_area(blocks);

    // Sample one block and put the cell into it
    const size_t gi = sample(indexes);
    auto& sampled = blocks[gi];

    sampled.cells.push_back(i);
    sampled.area += inputs.cell_areas[i];

    // Return nullopt upon block area violation
    if (sampled.area > inputs.max_block_area) {
      return std::nullopt;
    }
  }

  return blocks;
}

size_t next_k(size_t k, size_t ncells) {
  constexpr double growth = 1.1;

  size_t k1 = static_cast<size_t>(static_cast<double>(k) * growth);
  size_t k2 = k + 1;
  size_t k3 = ncells;
  size_t next = std::min(std::max(k1, k2), k3);
  fmt::print("Retry with k = {}\n", next);
  return next;
}

}  // namespace

// Finds an starting partition with unspecified (but legal) #blocks.
// Throws if it is impossible to partition.
vector<Block> find_starting_partition(const InputData& inputs) noexcept(false) {
  // TODO: Find better strategy than "k += 1"
  for (size_t k = inputs.min_number_of_blocks(); k <= inputs.ncells;
       k = next_k(k, inputs.ncells)) {
    if (auto blocks = find_starting_partition(inputs, k)) {
      fmt::print("Success with {}-way partition\n", k);
      return *blocks;
    }
    fmt::print("Failed to find starting partition with k = {}\n", k);
  }
  throw std::runtime_error("Failed to find starting partition");
}
