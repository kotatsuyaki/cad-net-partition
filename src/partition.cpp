#include "config.hpp"
#include "cost.hpp"
#include "data.hpp"
#include "partition.hpp"

#include <fmt/chrono.h>
#include <gsl/narrow>
#include <parallel_hashmap/phmap.h>
#include <range/v3/action/remove.hpp>
#include <range/v3/all.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>
#include <type_traits>
#include <vector>

using gsl::narrow;
using gsl::narrow_cast;
using ranges::to;
using ranges::views::enumerate;
using ranges::views::transform;
using std::vector;

using namespace std::chrono_literals;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::steady_clock;

template <typename K, typename V>
using map = phmap::flat_hash_map<K, V>;

template <typename T>
using set = phmap::flat_hash_set<T>;

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

// Auto-adapting temperature factor that approaches `temp_limit` at time
// `time_limit`.
class TempFactor {
 public:
  TempFactor(double init_temp_factor = config::default_init_temp_factor)
      : temp_factor(init_temp_factor), last_update_time(steady_clock::now()) {}

  // Gets the factor.
  double operator()() const { return temp_factor; }

  // Updates the factor. This should be called every time when a pass is
  // performed.
  void update(steady_clock::time_point begin_time, double current_temp) {
    passes += 1;
    const bool should_update = steady_clock::now() - last_update_time >
                               config::temp_factor_update_interval;
    if (should_update == false) {
      return;
    }

    const auto remaining_time =
        config::time_limit - (steady_clock::now() - begin_time);
    const double remain_secs =
        static_cast<double>(duration_cast<seconds>(remaining_time).count());
    const double passes_per_sec = static_cast<double>(passes) / 10.0;

    temp_factor = std::pow(config::temp_limit / current_temp,
                           1.0 / (remain_secs * passes_per_sec));

    // Correct factor if it goes crazy
    if (temp_factor <= 0.0) {
      temp_factor = config::default_init_temp_factor;
    }

    passes = 0;
    last_update_time = steady_clock::now();
  }

 private:
  double temp_factor;

  steady_clock::time_point last_update_time;
  int64_t passes = 0;
};

// Random generator supplying random numbers for `SimAnneal`.
class Random {
 public:
  Random(size_t ncells, size_t nblocks)
      : rd(),
        gen(rd()),
        cell_id_gen(0, ncells - 1),
        block_id_gen(0, nblocks - 1),
        zero_one_gen(0.0, 1.0) {}
  CellId cell_id() { return cell_id_gen(gen); }
  BlockId block_id() { return block_id_gen(gen); }
  double zero_to_one() { return zero_one_gen(gen); }

 private:
  std::random_device rd;
  std::mt19937 gen;
  std::uniform_int_distribution<CellId> cell_id_gen;
  std::uniform_int_distribution<BlockId> block_id_gen;
  std::uniform_real_distribution<double> zero_one_gen;
};

class SimAnneal {
 public:
  SimAnneal(const std::vector<Block>& blocks, const InputData& inputs,
            Cost init_cost, double init_temp = config::default_init_temp)
      : blocks(blocks),
        cost(init_cost),
        temp(init_temp),
        temp_factor(),
        random(inputs.ncells, blocks.size()),
        begin_time(steady_clock::now()) {
    block_of_cell = blocks_to_block_of_cell(blocks, inputs.ncells);
    populate_span_of_net(inputs);
    populate_bindings(inputs);
  }

  bool should_terminate() const {
    const bool is_time_over =
        (steady_clock::now() - begin_time) > config::time_limit;
    return is_time_over;
  }

  enum class PassStatus {
    Success,
    Abort,
    UphillReject,
  };

  struct PassResult {
    PassStatus status;
    Cost cost;
    Cost cost_delta;
    double temp;
    double temp_factor;
  };

  PassResult perform_pass(const InputData& inputs) {
    const CellId cell_id = random.cell_id();
    const BlockId from_block_id = block_of_cell[cell_id];
    const BlockId to_block_id = random.block_id();

    const bool legal = blocks[to_block_id].area + inputs.cell_areas[cell_id] <=
                       inputs.max_block_area;
    const bool is_not_move = from_block_id == to_block_id;
    if (legal == false || is_not_move == true) {
      return {PassStatus::Abort, 0, 0, temp, temp_factor()};
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
        random.zero_to_one() <=
        std::exp(narrow_cast<double>(-cost_delta) / temp);

    if (is_downhill == false && is_rand_accept == false) {
      return {PassStatus::UphillReject, 0, cost_delta, temp, temp_factor()};
    }

    // accepted; update records
    cost += cost_delta;

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

    // Move the cell
    block_of_cell[cell_id] = to_block_id;
    blocks[from_block_id].cells |= ranges::actions::remove(cell_id);
    blocks[from_block_id].area -= inputs.cell_areas[cell_id];
    blocks[to_block_id].cells.emplace_back(cell_id);
    blocks[to_block_id].area += inputs.cell_areas[cell_id];

    // Update and clamp temperature
    temp *= temp_factor();
    temp = std::clamp(temp, config::temp_limit, config::temp_limit_top);

    temp_factor.update(begin_time, temp);

    return PassResult{PassStatus::Success, cost, cost_delta, temp,
                      temp_factor()};
  }

  // Gets the resulting blocks and destroys it.
  // It is not allowed to do anything with this instance of `SimAnneal` after
  // calling this method.
  vector<Block> into_blocks() { return std::move(blocks); }

 private:
  vector<Block> blocks;

  // (BlockId, NetId) -> Int
  map<Key, size_t> bindings;

  // CellId -> BlockId
  vector<BlockId> block_of_cell;

  // NetId -> Int (#blocks spanned by net)
  vector<Cost> span_of_net;

  Cost cost;
  double temp;
  TempFactor temp_factor;
  Random random;

  steady_clock::time_point begin_time;

  void populate_span_of_net(const InputData& inputs) {
    vector<set<BlockId>> blocks_of_net(inputs.nnets);
    for (const auto& [block_id, block] : blocks | enumerate) {
      for (const CellId cell_id : block.cells) {
        for (const NetId net_id : inputs.cells.at(cell_id)) {
          blocks_of_net.at(net_id).insert(block_id);
        }
      }
    }

    // Transform blocks_of_net to span sizes
    span_of_net = blocks_of_net | transform([](const set<BlockId>& bs) {
                    return narrow<Cost>(bs.size());
                  }) |
                  to<vector<Cost>>();
  }

  void populate_bindings(const InputData& inputs) {
    for (const auto& [block_id, block] : blocks | enumerate) {
      for (const CellId cell_id : block.cells) {
        const auto& cell = inputs.cells[cell_id];
        for (const NetId net_id : cell) {
          bindings[{block_id, net_id}] += 1;
        }
      }
    }
  }
};

class ProgressReporter {
 public:
  ProgressReporter(Cost init_cost)
      : last_update_time(steady_clock::now()),
        begin_time(steady_clock::now()),
        init_cost(init_cost) {}

  // Updates value stored in the reporter.
  // If elapsed time since last print is long enough, print out info.
  void update(const SimAnneal::PassResult& res) {
    if (res.status == SimAnneal::PassStatus::Success) {
      num_success += 1;
    }

    const bool is_time_to_print =
        steady_clock::now() - last_update_time > config::report_interval;
    if (is_time_to_print == false) {
      return;
    }

    const double pass_per_sec =
        static_cast<double>(num_success) /
        static_cast<double>(
            duration_cast<seconds>(config::report_interval).count());
    const double opt_rate =
        static_cast<double>(res.cost) / static_cast<double>(init_cost) * 100.0;
    fmt::print(
        "Cost {:>10}  |  Opt {:<7.3}%  |  Elapsed {:%H:%M:%S}  |  Temp "
        "{:<12.9}  |  TempFactor "
        "{:<12.9}  |  PassPerSec {:<12.9}\n",
        res.cost, opt_rate, steady_clock::now() - begin_time, res.temp,
        res.temp_factor, pass_per_sec);
    fflush(stdout);

    last_update_time = steady_clock::now();
    num_success = 0;
  }

 private:
  steady_clock::time_point last_update_time;
  steady_clock::time_point begin_time;
  int64_t num_success = 0;
  Cost init_cost;
};

}  // namespace

std::vector<Block> perform_sa_partition(const std::vector<Block>& blocks,
                                        const InputData& inputs,
                                        Cost init_cost) {
  SimAnneal sim_anneal{blocks, inputs, init_cost};
  ProgressReporter reporter{init_cost};

  while (sim_anneal.should_terminate() == false) {
    const auto res = sim_anneal.perform_pass(inputs);
    if (res.status == SimAnneal::PassStatus::Success) {
      reporter.update(res);
    }
  }

  const auto optimized_blocks = sim_anneal.into_blocks();
  return optimized_blocks;
}
