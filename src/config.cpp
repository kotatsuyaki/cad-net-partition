#include "config.hpp"

#define FMT_HEADER_ONLY
#include <fmt/core.h>

#include <cstdlib>

Config::Config() {
  if (std::getenv("PA2_DEBUG_INPUTS")) {
    fmt::print("PA2_DEBUG_INPUTS is set\n");
    debug_inputs = true;
  }

  if (std::getenv("PA2_VERIFY_BLOCKS")) {
    fmt::print("PA2_VERIFY_BLOCKS is set\n");
    verity_blocks = true;
  }
}
