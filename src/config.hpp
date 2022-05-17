#ifndef CONFIG_HPP_
#define CONFIG_HPP_

namespace {
constexpr int default_rounds = 10;
}

struct Config {
  // Constructs a `Config` from environment variables.
  Config();

  // Whether to print detailed debug information about moves during passes.
  bool debug_moves = false;

  // Whether to print inputs upon start.
  bool debug_inputs = false;

  // Rounds of passes to perform.
  int pass_rounds = default_rounds;

  // Whether to verify partitions.
  bool verity_blocks = false;

  // Allow k-way (k > 2) partitioning.
  bool allow_kway = false;
};

#endif  // CONFIG_HPP_
