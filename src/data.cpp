#include "data.hpp"

#define FMT_HEADER_ONLY
#include <fmt/ostream.h>
#include <parallel_hashmap/phmap.h>
#include <range/v3/all.hpp>

#include <cmath>
#include <stdexcept>
#include <vector>

using std::istream;
using std::string;
using std::vector;

using ranges::to;
using ranges::actions::sort;
using ranges::views::enumerate;
using ranges::views::transform;

template <typename T>
using set = phmap::parallel_flat_hash_set<T>;

InputData InputData::read_from(std::istream& is) noexcept(false) {
  InputData data;
  data.read(is);
  return data;
}

void InputData::read(istream& is) noexcept(false) {
  // enable exceptions on input errors
  is.exceptions(istream::failbit | istream::badbit);

  cell_areas.clear();
  nets.clear();

  string ignore_word;

  is >> max_block_area >> ignore_word;
  if (ignore_word != ".cell") {
    throw std::runtime_error("expects keyword '.cell'");
  }
  is >> ncells;

  cell_areas.resize(ncells);
  for (size_t i = 0; i < ncells; i += 1) {
    size_t index = 0;
    is >> index;
    is >> cell_areas[index];
  }

  is >> ignore_word;
  if (ignore_word != ".net") {
    throw std::runtime_error("expects keyword '.net'");
  }

  is >> nnets;
  nets.resize(nnets);
  cells.resize(ncells);

  // `net` is the index of net
  for (size_t net = 0; net < nnets; net += 1) {
    size_t ncells_contained = 0;
    is >> ncells_contained;

    for (size_t i = 0; i < ncells_contained; i += 1) {
      size_t cell = 0;
      is >> cell;
      nets.at(net).insert(cell);
      cells.at(cell).insert(net);
    }
  }

  // Calculate total area
  total_area = ranges::accumulate(cell_areas, 0);

  // Find max number of nets per cell
  max_nets_per_cell = ranges::max(
      cells | transform([](const Cell& cell) { return cell.size(); }));

  return;
}

size_t InputData::min_number_of_blocks() const {
  return std::ceil(static_cast<double>(total_area) /
                   static_cast<double>(max_block_area));
}

void InputData::debug_print() const {
  fmt::print("{}\n", *this);
  fmt::print("Cell areas: {}\n", fmt::join(cell_areas, ", "));
  for (const auto& [i, net] : nets | enumerate) {
    fmt::print("Net {} contains cells: {}\n", i,
               fmt::join(net | to<std::vector>() | sort, ", "));
  }
}

vector<BlockId> blocks_to_block_of_cell(const vector<Block>& blocks,
                                        size_t ncells) {
  vector<BlockId> block_of_cell(ncells);
  for (const auto& [block_id, block] : blocks | enumerate) {
    for (const CellId cell_id : block.cells) {
      block_of_cell[cell_id] = block_id;
    }
  }
  return block_of_cell;
}

void verify_blocks(const std::vector<Block>& blocks, size_t ncells) {
  // Verify that no cell-id appears twice
  set<CellId> seen_cells;
  for (const auto& [block_id, block] : blocks | enumerate) {
    for (const CellId cell_id : block.cells) {
      if (seen_cells.contains(cell_id)) {
        throw std::runtime_error(
            fmt::format("Duplicated cell {} found in verification", cell_id));
      }
      seen_cells.emplace(cell_id);
    }
  }

  // Verify that every cell-id appreas
  if (seen_cells.size() != ncells) {
    throw std::runtime_error(
        fmt::format("Some cells are not found in verification ({} != {})",
                    seen_cells.size(), ncells));
  }
}

void write_blocks(std::ostream& os, size_t cost,
                  const std::vector<Block>& blocks, const InputData& inputs) {
  fmt::print(os, "{}\n{}\n", cost, blocks.size());

  const auto block_of_cell = blocks_to_block_of_cell(blocks, inputs.ncells);
  for (const auto& [cell_id, cell] : inputs.cells | enumerate) {
    fmt::print(os, "{}\n", block_of_cell[cell_id]);
  }
}
