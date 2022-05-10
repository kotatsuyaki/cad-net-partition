#include "cost.hpp"

#include <parallel_hashmap/phmap.h>
#include <range/v3/all.hpp>

#include <vector>

using phmap::flat_hash_set;
using ranges::accumulate;
using ranges::views::enumerate;
using ranges::views::transform;
using std::vector;

size_t find_cost(const vector<Group>& groups, const InputData& inputs) {
  vector<flat_hash_set<size_t>> spans(inputs.nnets);

  for (const auto& [gi, group] : groups | enumerate) {
    for (size_t c : group.cells) {
      for (size_t net : inputs.cells.at(c)) {
        spans.at(net).insert(gi);
      }
    }
  }

  const auto get_size = [](const auto& span) { return span.size(); };
  const auto net_cost = [](size_t count) { return (count - 1) * (count - 1); };
  return ranges::accumulate(spans | transform(get_size) | transform(net_cost),
                            0);
}
