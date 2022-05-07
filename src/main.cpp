#include "io.hpp"

#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <fmt/format.h>
#include <gsl/gsl>
#include <range/v3/all.hpp>

#include <cassert>
#include <fstream>

namespace views = ranges::views;
namespace actions = ranges::actions;
using ranges::to;

int main(int argc, char** argv) {
  assert(argc >= 2);
  std::ifstream infile(argv[1]);

  const auto inputs = InputData::read_from(infile);

  fmt::print("Cell areas: {}\n", fmt::join(inputs.cell_areas, ", "));
  for (const auto& [i, net] : inputs.nets | views::enumerate) {
    fmt::print("Net {} contains cells: {}\n", i,
               fmt::join(net | to<std::vector>() | actions::sort, ", "));
  }

  return 0;
}
