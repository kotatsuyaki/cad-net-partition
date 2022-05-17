#include "config.hpp"
#include "cost.hpp"
#include "data.hpp"
#include "partition.hpp"
#include "sa.hpp"
#include "starting_partition.hpp"

#include <limits>

#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <range/v3/all.hpp>

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <stdexcept>

using ranges::views::enumerate;
using std::ifstream;
using std::ofstream;
using std::string;
using std::vector;

void debug_print_inputs(const InputData& inputs, const vector<Block>& blocks);

int main(int argc, char** argv) try {
  const Config config{};

  if (argc < 3) {
    throw std::runtime_error("No enough arguments\n");
  }

  ifstream infile(argv[1]);  // NOLINT

  // Read input and find starting partitioning
  const InputData inputs = InputData::read_from(infile);
  vector<Block> blocks = find_starting_partition(inputs);

  if (config.debug_inputs) {
    debug_print_inputs(inputs, blocks);
  }

  // Record best partitioning so far
  auto best_blocks(blocks);
  auto best_cost = find_cost(blocks, inputs);

  fmt::print("Cost of starting partition = {}\n", best_cost);

  if (false) {
    // if (blocks.size() == 2 || config.allow_kway) {
    // Bail after fixed number of useless moves
    constexpr int bad_rounds_limit = 10;

    int bad_rounds = 0;
    while (bad_rounds < bad_rounds_limit) {
      const auto new_blocks = perform_pass(blocks, inputs);
      const auto cost = find_cost(new_blocks, inputs);

      if (config.verity_blocks) {
        verify_blocks(new_blocks, inputs.ncells);
      }

      fmt::print("Cost of new partition = {}\n", cost);
      blocks = new_blocks;

      if (cost < best_cost) {
        // Update best cost and reset bad rounds counter
        bad_rounds = 0;
        best_cost = cost;
        best_blocks = std::move(new_blocks);
      } else {
        // Increase bad rounds counter
        bad_rounds += 1;
        fmt::print("{} bad rounds\n", bad_rounds);
      }
      fmt::print("best = {}\n", best_cost);
    }
  } else {
    fmt::print("Multi-way optimization by Sanchis method disabled\n");
    best_blocks = perform_sa_partition(blocks, inputs);
    best_cost = find_cost(best_blocks, inputs);
    fmt::print("Cost after SA = {}\n", best_cost);
  }

  // Optionally verify the answer
  if (config.verity_blocks) {
    verify_blocks(best_blocks, inputs.ncells);
  }

  // Write output
  ofstream outfile(argv[2]);  // NOLINT

  fmt::print("Writing output\n");
  write_blocks(outfile, best_cost, best_blocks, inputs);
  outfile.flush();
  outfile.close();

  return 0;
} catch (const std::exception& e) {
  fmt::print(stderr, "Exception caught at main(): {}\n", e.what());
  return 1;
}

void debug_print_inputs(const InputData& inputs, const vector<Block>& blocks) {
  inputs.debug_print();
  for (const auto& [i, block] : blocks | enumerate) {
    fmt::print("Group {} (area = {}) contains cells: {}\n", i, block.area,
               fmt::join(block.cells, ", "));
  }

  const auto max_degree =
      ranges::max(inputs.cells | ranges::views::transform([](const Cell& cell) {
                    return cell.size();
                  }));
  fmt::print("Max degree p = {}\n", max_degree);
}
