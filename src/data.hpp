#ifndef DATA_HPP_
#define DATA_HPP_

#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <gsl/gsl>
#include <parallel_hashmap/phmap.h>

#include <istream>
#include <ostream>
#include <vector>

using NetId = size_t;
using CellId = size_t;
using BlockId = size_t;

using Cell = phmap::flat_hash_set<NetId>;
using Net = phmap::flat_hash_set<CellId>;

struct InputData {
  static InputData read_from(std::istream& is) noexcept(false);
  void read(std::istream& is) noexcept(false);
  size_t min_number_of_blocks() const;
  void debug_print() const;

  size_t max_block_area = 0;
  size_t ncells = 0;
  size_t nnets = 0;

  size_t max_nets_per_cell = 0;

  // Area of cells, indexed by cell numbers
  std::vector<size_t> cell_areas;

  // Hyperedges, indexed by net numbers
  std::vector<Net> nets;

  // Mapping from cell index to net indices
  std::vector<Cell> cells;

  size_t total_area = 0;
};

template <>
struct fmt::formatter<InputData> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end) it++;
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }

  template <typename FormatContext>
  auto format(const InputData& inputs, FormatContext& ctx)
      -> decltype(ctx.out()) {
    return format_to(ctx.out(),
                     "InputData({} cells, {} nets, max block area {})",
                     inputs.ncells, inputs.nnets, inputs.max_block_area);
  }
};

struct Block {
  Block() : cells() {}

  // Total area of the block
  size_t area = 0;

  // Cells contained in this block
  std::vector<CellId> cells;
};

// Total order of `Block` by area
inline bool operator<(const Block& ga, const Block& gb) {
  return ga.area < gb.area;
}
inline bool operator>(const Block& ga, const Block& gb) {
  return ga.area > gb.area;
}
inline bool operator<=(const Block& ga, const Block& gb) {
  return ga.area <= gb.area;
}
inline bool operator>=(const Block& ga, const Block& gb) {
  return ga.area >= gb.area;
}
inline bool operator==(const Block& ga, const Block& gb) {
  return ga.area == gb.area;
}
inline bool operator!=(const Block& ga, const Block& gb) {
  return ga.area != gb.area;
}

// Verifies whether `blocks` is a legal partitioning.
// Throws if the partitioning is illegal.
void verify_blocks(const std::vector<Block>& blocks, size_t ncells);

// Invert blocks (block-to-cell) as cell-to-block mapping
std::vector<BlockId> blocks_to_block_of_cell(const std::vector<Block>& blocks,
                                             size_t ncells);

void write_blocks(std::ostream& os, size_t cost,
                  const std::vector<Block>& blocks, const InputData& inputs);

#endif  // DATA_HPP_
