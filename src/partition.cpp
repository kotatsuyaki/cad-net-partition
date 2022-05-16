#include "config.hpp"
#include "partition.hpp"

#include <gsl/gsl>
#include <gsl/narrow>
#include <parallel_hashmap/phmap.h>
#include <range/v3/action/remove.hpp>
#include <range/v3/all.hpp>

#include <csignal>
#include <deque>
#include <iterator>
#include <limits>
#include <list>
#include <optional>
#include <vector>

using gsl::narrow;
using ranges::views::enumerate;
using std::deque;
using std::list;
using std::nullopt;
using std::optional;
using std::pair;
using std::vector;

template <typename K, typename V>
using map = phmap::flat_hash_map<K, V>;

template <typename T>
using set = phmap::flat_hash_set<T>;

namespace {

constexpr size_t infty = std::numeric_limits<int>::max();
constexpr int max_level = 2;

// Storage for phi, lambda, and beta values.
// Indexed by net and block.
class BindData {
 public:
  BindData(size_t nnets) : data(nnets) {}

  int operator()(NetId net, BlockId block) const {
    assert(net < data.size());

    auto it = data[net].find(block);
    // defaults to 0
    if (it == data[net].end()) {
      return 0;
    }
    return it->second;
  }

  // Increase value at (net, block) by 1.
  // An unset value defaults to 0, so after the increment it becomes 1.
  void inc(NetId net, BlockId block) {
    assert(net < data.size());
    if (data[net][block] == infty) {
      return;
    }
    data[net][block] += 1;
  }

  void dec(NetId net, BlockId block) {
    assert(net < data.size());
    if (data[net][block] == infty) {
      return;
    }
    data[net][block] -= 1;
  }

  // Returns number of changed infty values
  [[nodiscard]] int set(NetId net, BlockId block, int value) {
    assert(net < data.size());

    const size_t old = data[net][block];
    data[net][block] = value;

    if (old != infty && value == infty) {
      return 1;
    }
    if (old == infty && value != infty) {
      return -1;
    }
    return 0;
  }

 private:
  // indexed by net, and then block
  vector<map<BlockId, int>> data;
};

struct Move {
  CellId cell;
  BlockId to_block;
};

using Gain = pair<int, int>;

// Data structure mapping moves to gain vectors
class GainValues {
 public:
  GainValues(size_t ncells, size_t nblocks)
      : data(ncells * nblocks), ncells(ncells), nblocks(nblocks) {}

  Gain operator[](Move move) const { return data[to_index(move)]; }

  Gain inc(Move move, size_t level) {
    update<1>(move, level);
    return (*this)[move];
  }

  Gain dec(Move move, size_t level) {
    update<-1>(move, level);
    return (*this)[move];
  }

 private:
  // indexed by (CellId, BlockId)
  vector<Gain> data;
  size_t ncells, nblocks;

  template <int diff>
  void update(Move move, size_t level) {
    if (level == 1) {
      data[to_index(move)].first += diff;
    } else if (level == 2) {
      data[to_index(move)].second += diff;
    }
  }

  size_t to_index(Move move) const {
    const auto [cell_id, to_block_id] = move;
    return cell_id * nblocks + to_block_id;
  }
};

// Table mapping gain vectors to moves.
class GainTable {
 public:
  using Entry = vector<pair<CellId, BlockId>>;

  GainTable(size_t table_size, int p)
      : data(table_size * table_size),
        table_size(table_size),
        p(p),
        max_gain_it(data.crbegin()) {}
  void add(Gain gain, CellId cell_id, BlockId to_block_id) {
    data[to_index(gain)].emplace_back(cell_id, to_block_id);

    auto entry_it =
        std::next(data.crbegin(), gsl::narrow_cast<ptrdiff_t>(to_index(gain)));
    if (entry_it < max_gain_it) {
      max_gain_it = entry_it;
    }
  }

  void remove(Gain gain, CellId cell_id, BlockId to_block_id) {
    Entry& entry = data[to_index(gain)];
    const auto it = ranges::find(entry, std::make_pair(cell_id, to_block_id));
    if (it != entry.end()) {
      entry.erase(it);
    }

    // Update max gain iterator if needed
    auto entry_it =
        std::next(data.crbegin(), gsl::narrow_cast<ptrdiff_t>(to_index(gain)));
    if (entry_it == max_gain_it && entry.empty()) {
      max_gain_it = search_down_max_gain();
    }
  }

  // Iterates in descending order.
  auto begin() const { return max_gain_it; }
  auto end() const { return data.crend(); }

 private:
  // indexed by (first level gain) * (2p + 1) + second level gain
  vector<Entry> data;
  size_t table_size;

  // minimum gain scalar is -p
  int p;

  // Max gain in the table
  decltype(data.crbegin()) max_gain_it;

  // Find new max gain down from the current one
  [[nodiscard]] decltype(data.crbegin()) search_down_max_gain() {
    for (auto it = max_gain_it; it != data.crend(); it++) {
      if (it->empty() == false) {
        return it;
      }
    }
    return data.crend();
  }

  size_t to_index(Gain gain) const {
    auto [f, s] = gain;
    f += p;
    s += p;
    return f * table_size + s;
  }
};

enum class NetStatus : uint8_t { Free, Loose, Locked };

class Cutter {
 public:
  Cutter(const InputData& inputs, const vector<Block>& blocks)
      : config(),
        nets(inputs.nets),
        phi(inputs.nnets),
        lmd(inputs.nnets),
        beta(inputs.nnets),
        infty_count(inputs.nnets),
        cells(inputs.cells),
        block_of_cell(),
        cell_locked(inputs.ncells),
        area_of_cell(inputs.cell_areas),
        blocks(blocks),
        gains(inputs.ncells, blocks.size()),
        gain_table(inputs.max_nets_per_cell * 2 + 1,
                   gsl::narrow<int>(inputs.max_nets_per_cell)),
        max_block_area(inputs.max_block_area) {
    populate_block_of_cell();

    // Init-partition 2)
    for (const auto& [cell_id, cell] : cells | enumerate) {
      const BlockId block_id = block_of_cell[cell_id];
      for (const auto& [net_id, net] : cell | enumerate) {
        phi.inc(net_id, block_id);
        beta.inc(net_id, block_id);
      }
    }

    // Init-partition 3)
    for (const auto& [net_id, net] : nets | enumerate) {
      for (const auto& [block_id, block] : blocks | enumerate) {
        const bool betap_small = betap(net_id, block_id) <= max_level;
        const bool beta_nz = beta(net_id, block_id) > 0;
        if (betap_small && beta_nz) {
          // update gain for all cells on net
          for (const auto& [cell_id, cell] : net | enumerate) {
            update_gain<Normal>(cell_id, block_id, net_id);
          }
        }
      }
    }

    // Init-partition 4)
    for (const auto& [cell_id, cell] : cells | enumerate) {
      const BlockId cell_block_id = block_of_cell[cell_id];
      for (const auto& [block_id, block] : blocks | enumerate) {
        if (block_id == cell_block_id) {
          continue;
        }

        const Gain gain = gains[{cell_id, block_id}];
        gain_table.add(gain, cell_id, block_id);
      }
    }
  }

  std::vector<Move> perform_pass() {
    const size_t min_moves = cell_locked.size() / 8;

    vector<Move> move_history;
    vector<int> gain_history;
    int current_gain = 0;

    size_t count = 0;
    while (const auto move_opt = find_nextmove()) {
      const Move move = *move_opt;
      const Gain gain = gains[{move.cell, move.to_block}];

      if (gain.first <= 0 && count >= min_moves) {
        fmt::print("First-level gain = {} <= 0, aborting\n", gain.first);
        break;
      }

      const BlockId to_block = move.to_block;
      const BlockId from_block = block_of_cell[move.cell];

      if (config.debug_moves) {
        fmt::print(
            "[{}] Move cell {} from block {} to block {} (gain = ({}, {}))\n",
            count, move.cell, from_block, to_block, gain.first, gain.second);
      }
      count += 1;

      perform_move(move);

      // Update block records
      blocks[from_block].area -= area_of_cell[move.cell];
      ranges::remove(blocks[from_block].cells, move.cell);
      blocks[to_block].area += area_of_cell[move.cell];
      blocks[to_block].cells.push_back(move.cell);
      block_of_cell[move.cell] = to_block;

      current_gain += gain.first;
      gain_history.emplace_back(current_gain);
      move_history.emplace_back(move);
    }

    // Return partial history up to best gain
    const auto max_it =
        ranges::max_element(gain_history, [](int a, int b) { return a <= b; });
    if (max_it != gain_history.end()) {
      const auto max_index = std::distance(gain_history.begin(), max_it);
      move_history.resize(max_index + 1);
    }
    return move_history;
  }

  void replay(vector<Block>& ext_blocks, const vector<Move>& moves) const {
    fmt::print("Replaying {} moves\n", moves.size());
    auto block_of_cell =
        blocks_to_block_of_cell(ext_blocks, cell_locked.size());

    for (const auto [cell_id, to_block] : moves) {
      const BlockId from_block = block_of_cell[cell_id];

      ext_blocks[from_block].area -= area_of_cell[cell_id];
      ext_blocks[to_block].area += area_of_cell[cell_id];

      ext_blocks[from_block].cells |= ranges::actions::remove(cell_id);
      ext_blocks[to_block].cells.push_back(cell_id);

      // NOTE: Remove this
      assert(ranges::none_of(
          ext_blocks[from_block].cells,
          [cell_id = cell_id](CellId id) { return id == cell_id; }));

      block_of_cell[cell_id] = to_block;
    }
  }

 private:
  Config config;

  // data indexed by net
  vector<Net> nets;
  BindData phi, lmd, beta;
  vector<int> infty_count;

  // data indexed by cell
  vector<Cell> cells;
  vector<BlockId> block_of_cell;
  vector<bool> cell_locked;
  vector<size_t> area_of_cell;

  // data indexed by block
  vector<Block> blocks;

  // gain structures
  GainValues gains;
  GainTable gain_table;

  size_t max_block_area;

  int betap(NetId net, BlockId block) const {
    const int net_size = narrow<int>(nets[net].size());
    switch (net_status(net)) {
      case NetStatus::Free:
        return net_size - phi(net, block);
      case NetStatus::Locked:
        return infty;
      case NetStatus::Loose:
        return net_size - phi(net, block) - lmd(net, block);
      default:
        throw std::runtime_error("Got illegal NetStatus value");
    }
  }

  NetStatus net_status(NetId net_id) const {
    const auto count = infty_count[net_id];
    assert(count >= 0);
    if (count == 0) {
      return NetStatus::Free;
    } else if (count == 1) {
      return NetStatus::Loose;
    } else {
      return NetStatus::Locked;
    }
  }

  void populate_block_of_cell() {
    block_of_cell = blocks_to_block_of_cell(blocks, cell_locked.size());
  }

  struct Normal;
  struct Reverse;

  template <typename Mode>
  void update_gain(CellId cell_id, BlockId to_block_id, NetId net_id) {
    if (cell_locked[cell_id]) {
      return;
    }

    const BlockId from_block_id = block_of_cell[cell_id];
    if (from_block_id != to_block_id) {
      const int i = betap(net_id, to_block_id);

      if constexpr (std::is_same_v<Mode, Normal>) {
        increase_gain(cell_id, to_block_id, i);
      } else {
        decrease_gain(cell_id, to_block_id, i);
      }
    } else if (betap(net_id, to_block_id) < max_level) {
      const int i = betap(net_id, to_block_id) + 1;
      for (const auto& [block_id, block] : blocks | enumerate) {
        if (block_id == from_block_id) {
          continue;
        }

        if constexpr (std::is_same_v<Mode, Normal>) {
          decrease_gain(cell_id, block_id, i);
        } else {
          increase_gain(cell_id, block_id, i);
        }
      }
    }
  }

  void increase_gain(CellId cell_id, BlockId to_block_id, size_t i) {
    const Gain old_gain = gains[{cell_id, to_block_id}];
    const Gain new_gain = gains.inc({cell_id, to_block_id}, i);

    gain_table.remove(old_gain, cell_id, to_block_id);
    gain_table.add(new_gain, cell_id, to_block_id);
  }

  void decrease_gain(CellId cell_id, BlockId to_block_id, size_t i) {
    const Gain old_gain = gains[{cell_id, to_block_id}];
    const Gain new_gain = gains.dec({cell_id, to_block_id}, i);

    gain_table.remove(old_gain, cell_id, to_block_id);
    gain_table.add(new_gain, cell_id, to_block_id);
  }

  optional<Move> find_nextmove() const {
    for (const auto& entry : gain_table) {
      for (const auto& [cell_id, to_block_id] : entry) {
        // check if cell is locked
        if (cell_locked[cell_id]) {
          continue;
        }

        // check if moving cell to block is viable
        const size_t area_after_move =
            blocks[to_block_id].area + area_of_cell[cell_id];
        if (area_after_move > max_block_area) {
          continue;
        }

        return Move{cell_id, to_block_id};
      }
    }
    return nullopt;
  }

  void perform_move(Move move) {
    const auto [cell_id, to_block_id] = move;
    const BlockId from_block_id = block_of_cell[cell_id];

    // Set cell to be locked
    cell_locked[cell_id] = true;

    // Remove `cell_id`-related entries from gain structures
    for (const auto& [block_id, block] : blocks | enumerate) {
      const Gain gain = gains[{cell_id, block_id}];
      gain_table.remove(gain, cell_id, to_block_id);
    }

    // For each net `net_id` connected to the moved cell,
    // consider free cells under the same net.
    Cell& cell = cells[cell_id];
    for (const NetId net_id : cell) {
      // pre update
      for (const auto& [block_id, nei_block] : blocks | enumerate) {
        const bool betap_small = betap(net_id, block_id) <= max_level;
        const bool beta_nz = beta(net_id, block_id);

        if (betap_small && beta_nz) {
          const Net& net = nets[net_id];
          for (const CellId nei_cell_id : net) {
            if (nei_cell_id == cell_id) {
              continue;
            }

            update_gain<Reverse>(nei_cell_id, block_id, net_id);
          }
        }
      }

      // update binding numbers
      phi.dec(net_id, from_block_id);
      lmd.inc(net_id, to_block_id);
      if (lmd(net_id, from_block_id) == 0) {
        infty_count[net_id] +=
            beta.set(net_id, from_block_id, phi(net_id, from_block_id));
      } else {
        infty_count[net_id] += beta.set(net_id, from_block_id, infty);
      }

      if (lmd(net_id, to_block_id) == 0) {
        infty_count[net_id] +=
            beta.set(net_id, to_block_id, phi(net_id, to_block_id));
      } else {
        infty_count[net_id] += beta.set(net_id, to_block_id, infty);
      }

      // post update
      const bool betap_small = betap(net_id, to_block_id) <= max_level;
      const bool beta_nz = beta(net_id, to_block_id) > 0;
      if (betap_small && beta_nz) {
        const Net& net = nets[net_id];
        for (const CellId nei_cell_id : net) {
          if (nei_cell_id == cell_id) {
            continue;
          }
          if (cell_locked[nei_cell_id]) {
            continue;
          }

          update_gain<Normal>(nei_cell_id, to_block_id, net_id);
        }
      }
    }
  }
};

}  // namespace

vector<Block> perform_pass(const vector<Block>& blocks,
                           const InputData& inputs) {
  Cutter cutter(inputs, blocks);

  const auto moves = cutter.perform_pass();
  fmt::print("Got {} moves from cutter\n", moves.size());

  auto new_blocks(blocks);
  cutter.replay(new_blocks, moves);
  return new_blocks;
}
