#include "config.hpp"
#include "cost.hpp"
#include "data.hpp"
#include "partition.hpp"
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
  const auto starting_blocks = find_starting_partition(inputs);
  const auto starting_cost = find_cost(starting_blocks, inputs);

  fmt::print("Cost of starting partition = {}\n", starting_cost);
  if (config.debug_inputs) {
    debug_print_inputs(inputs, starting_blocks);
  }

  // Optimize
  const auto optimized_blocks =
      perform_sa_partition(starting_blocks, inputs, starting_cost);
  const auto optimized_cost = find_cost(optimized_blocks, inputs);
  fmt::print("Cost after SA = {}\n", optimized_cost);

  // Optionally verify the answer
  if (config.verity_blocks) {
    verify_blocks(optimized_blocks, inputs.ncells);
  }

  // Write output
  ofstream outfile(argv[2]);  // NOLINT

  fmt::print("Writing output to file {}\n", argv[2]);  // NOLINT
  write_blocks(outfile, optimized_cost, optimized_blocks, inputs);
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
