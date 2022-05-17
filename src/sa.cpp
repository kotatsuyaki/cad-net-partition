#include "cost.hpp"
#include "data.hpp"
#include "gsl/narrow"
#include "sa.hpp"

#include <parallel_hashmap/phmap.h>
#include <range/v3/action/remove.hpp>
#include <range/v3/all.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

using gsl::narrow;
using gsl::narrow_cast;
using ranges::to;
using ranges::views::enumerate;
using ranges::views::transform;
using std::vector;

template <typename K, typename V>
using map = phmap::flat_hash_map<K, V>;

template <typename T>
using set = phmap::flat_hash_set<T>;

using Cost = std::int64_t;

namespace {
struct Key {
  BlockId block_id;
  NetId net_id;

  friend size_t hash_value(const Key& key) {
    return phmap::HashState().combine(0, key.block_id, key.net_id);
  }
};

bool operator==(Key a, Key b) noexcept {
  return a.block_id == b.block_id && a.net_id == b.net_id;
}

template <typename T>
constexpr T sqr(T t) noexcept {
  return t * t;
}
}  // namespace

std::vector<Block> perform_sa_partition(const std::vector<Block>& blocks,
                                        const InputData& inputs) {
  Cost cost = narrow<Cost>(find_cost(blocks, inputs));
  auto new_blocks{blocks};

  // NetId -> {BlockId}
  vector<set<BlockId>> blocks_of_net(inputs.nnets);
  // (BlockId, NetId) -> Int
  map<Key, size_t> bindings;
  // CellId -> BlockId
  auto block_of_cell = blocks_to_block_of_cell(blocks, inputs.ncells);

  // Initialize spans
  for (const auto& [block_id, block] : blocks | enumerate) {
    for (const CellId cell_id : block.cells) {
      for (const NetId net_id : inputs.cells.at(cell_id)) {
        blocks_of_net.at(net_id).insert(block_id);
      }
    }
  }

  // NetId -> Int (#blocks spanned by net)
  auto span_of_net = blocks_of_net | transform([](const set<BlockId>& bs) {
                       return narrow<Cost>(bs.size());
                     }) |
                     to<vector<Cost>>();

  // Initialize bindings
  for (const auto& [block_id, block] : blocks | enumerate) {
    for (const CellId cell_id : block.cells) {
      const auto& cell = inputs.cells[cell_id];
      for (const NetId net_id : cell) {
        bindings[{block_id, net_id}] += 1;
      }
    }
  }

  constexpr double temp_factor = 0.9999999;
  constexpr double init_temp = 10.0;

  double temp = init_temp;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<CellId> cell_id_gen(0, inputs.ncells - 1);
  std::uniform_int_distribution<BlockId> block_id_gen(0, blocks.size() - 1);

  int64_t counter = 0;
  while (true) {
    if (temp <= 0.2) {
      break;
    }

    const CellId cell_id = cell_id_gen(gen);
    const BlockId from_block_id = block_of_cell[cell_id];
    const BlockId to_block_id = block_id_gen(gen);

    const bool legal =
        new_blocks[to_block_id].area + inputs.cell_areas[cell_id] <=
        inputs.max_block_area;
    const bool is_not_move = from_block_id == to_block_id;
    if (legal == false || is_not_move == true) {
      continue;
    }

    // Calculate change in cost
    Cost cost_delta = 0;
    for (const NetId net_id : inputs.cells[cell_id]) {
      int span_delta = 0;
      if (bindings[{from_block_id, net_id}] == 1) {
        // after moving cell away, net will no longer be spanning the block
        span_delta -= 1;
      }

      if (bindings[{to_block_id, net_id}] == 0) {
        // after moving cell in, net will now be (newly) spanning the block
        span_delta += 1;
      }

      const Cost old_span = span_of_net[net_id];
      const Cost new_span = old_span + span_delta;

      cost_delta += sqr(new_span - 1) - sqr(old_span - 1);
    }

    // determine to accept or reject
    const bool is_downhill = cost_delta < 0;
    std::uniform_real_distribution<double> rand_gen(0.0, 1.0);
    const bool is_rand_accept =
        rand_gen(gen) <= std::exp(narrow_cast<double>(-cost_delta) / temp);

    if (is_downhill == false && is_rand_accept == false) {
      // fmt::print("Reject with downhill cost delta = {}\n", cost_delta);
      continue;
    }

    // accepted; update records
    cost += cost_delta;
    counter += 1;
    if (counter % 1000 == 0) {
      fmt::print("Accept with cost delta = {} (temp = {:.4}, cost = {})\n",
                 cost_delta, temp, cost);
    }

    for (const NetId net_id : inputs.cells[cell_id]) {
      int span_delta = 0;
      if (bindings[{from_block_id, net_id}] == 1) {
        // after moving cell away, net will no longer be spanning the block
        span_delta -= 1;
      }
      bindings[{from_block_id, net_id}] -= 1;

      if (bindings[{to_block_id, net_id}] == 0) {
        // after moving cell in, net will now be (newly) spanning the block
        span_delta += 1;
      }
      bindings[{to_block_id, net_id}] += 1;

      if (span_delta != 0) {
        const Cost old_span = span_of_net[net_id];
        const Cost new_span = old_span + span_delta;
        span_of_net[net_id] = new_span;
      }
    }

    block_of_cell[cell_id] = to_block_id;
    new_blocks[from_block_id].cells |= ranges::actions::remove(cell_id);
    /* new_blocks[from_block_id].cells.erase(
        std::remove(new_blocks[from_block_id].cells.begin(),
                    new_blocks[from_block_id].cells.end(), cell_id),
        new_blocks[from_block_id].cells.end()); */
    new_blocks[from_block_id].area -= inputs.cell_areas[cell_id];
    new_blocks[to_block_id].cells.emplace_back(cell_id);
    new_blocks[to_block_id].area += inputs.cell_areas[cell_id];

    temp = temp * temp_factor;
  }

  return new_blocks;
}
