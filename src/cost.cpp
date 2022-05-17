#include "cost.hpp"
#include "data.hpp"

#include <parallel_hashmap/phmap.h>
#include <range/v3/all.hpp>

#include <vector>

using phmap::flat_hash_set;
using ranges::accumulate;
using ranges::views::enumerate;
using ranges::views::transform;
using std::vector;

Cost find_cost(const vector<Block>& blocks, const InputData& inputs) {
  auto block_of_cell = blocks_to_block_of_cell(blocks, inputs.ncells);
  vector<flat_hash_set<size_t>> spans(inputs.nnets);

  for (const auto& [block_id, block] : blocks | enumerate) {
    for (size_t cell_id : block.cells) {
      for (size_t net_id : inputs.cells.at(cell_id)) {
        spans.at(net_id).insert(block_id);
      }
    }
  }

  const auto get_size = [](const auto& span) { return span.size(); };
  const auto net_cost = [](size_t count) { return (count - 1) * (count - 1); };
  return ranges::accumulate(spans | transform(get_size) | transform(net_cost),
                            0);
}
