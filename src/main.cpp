#include "config.hpp"
#include "cost.hpp"
#include "data.hpp"
#include "partition.hpp"
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
using std::ofstream;
using std::string;
using std::vector;

int main(int argc, char** argv) try {
  const Config config{};

  assert(argc >= 3);

  ifstream infile(argv[1]);  // NOLINT

  // Read input and find starting partitioning
  const InputData inputs = InputData::read_from(infile);
  vector<Block> blocks = find_starting_partition(inputs);

  if (config.debug_inputs) {
    inputs.debug_print();
    for (const auto& [i, block] : blocks | enumerate) {
      fmt::print("Group {} (area = {}) contains cells: {}\n", i, block.area,
                 fmt::join(block.cells, ", "));
    }

    const auto max_degree = ranges::max(
        inputs.cells |
        ranges::views::transform([](const Cell& cell) { return cell.size(); }));
    fmt::print("Max degree p = {}\n", max_degree);
  }

  // Record best partitioning so far
  auto best_blocks(blocks);
  auto best_cost = find_cost(blocks, inputs);
  fmt::print("Cost of starting partition = {}\n", best_cost);

  if (blocks.size() == 2) {
    // Bail after 5 useless moves
    constexpr int bad_rounds_limit = 5;

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
        bad_rounds = 0;
        best_cost = cost;
        best_blocks = std::move(new_blocks);
      } else {
        bad_rounds += 1;
        fmt::print("{} bad rounds\n", bad_rounds);
      }
    }
  } else {
    fmt::print("Multi-way optimization disabled\n");
  }

  // Write output
  ofstream outfile(argv[2]);  // NOLINT
  if (config.verity_blocks) {
    verify_blocks(best_blocks, inputs.ncells);
  }
  write_blocks(outfile, best_cost, best_blocks, inputs);
  outfile.flush();
  outfile.close();

  return 0;
} catch (const std::exception& e) {
  fmt::print(stderr, "Exception caught at main(): {}\n", e.what());
  return 1;
}
