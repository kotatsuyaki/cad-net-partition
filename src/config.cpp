#include "config.hpp"

#define FMT_HEADER_ONLY
#include <fmt/core.h>

#include <cstdlib>

Config::Config() {
  if (std::getenv("PA2_DEBUG_MOVES")) {
    fmt::print("PA2_DEBUG_MOVES is set\n");
    debug_moves = true;
  }

  if (std::getenv("PA2_DEBUG_INPUTS")) {
    fmt::print("PA2_DEBUG_INPUTS is set\n");
    debug_inputs = true;
  }

  if (std::getenv("PA2_VERIFY_BLOCKS")) {
    fmt::print("PA2_VERIFY_BLOCKS is set\n");
    verity_blocks = true;
  }

  if (std::getenv("PA2_ALLOW_KWAY")) {
    fmt::print("PA2_ALLOW_KWAY is set\n");
    allow_kway = true;
  }

  if (auto cstr = std::getenv("PA2_PASS_ROUNDS")) {
    fmt::print("PA2_PASS_ROUNDS is set\n");
    std::string str{cstr};
    try {
      pass_rounds = std::stoi(str);
    } catch (const std::exception& e) {
      fmt::print("Failed to parse PA2_PASS_ROUNDS: {}\n", e.what());
    }
  }
}
