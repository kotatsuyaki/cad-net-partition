#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#include <chrono>

namespace {
constexpr int default_rounds = 10;
}

namespace config {
using namespace std::chrono_literals;

constexpr double default_init_temp = 10.0;
constexpr double default_init_temp_factor = 0.99999999999999;
constexpr double temp_limit = 0.2;
constexpr double temp_limit_top = 1.0;

constexpr std::chrono::steady_clock::duration temp_factor_update_interval = 10s;
constexpr std::chrono::steady_clock::duration report_interval = 10s;
constexpr std::chrono::steady_clock::duration time_limit = 10min;
}  // namespace config

struct Config {
  // Constructs a `Config` from environment variables.
  Config();

  // Whether to print inputs upon start.
  bool debug_inputs = false;

  // Whether to verify partitions.
  bool verity_blocks = false;
};

#endif  // CONFIG_HPP_
