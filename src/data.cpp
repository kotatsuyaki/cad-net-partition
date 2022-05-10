#include "data.hpp"

#include <range/v3/all.hpp>

#include <cmath>
#include <stdexcept>

using std::istream;
using std::string;

using ranges::to;
using ranges::actions::sort;
using ranges::views::enumerate;

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

  is >> max_group_area >> ignore_word;
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

  // `i` is the index of net
  for (size_t i = 0; i < nnets; i += 1) {
    size_t ncells_contained = 0;
    is >> ncells_contained;

    for (size_t j = 0; j < ncells_contained; j += 1) {
      size_t cell = 0;
      is >> cell;
      nets[i].insert(cell);
    }
  }

  // Calculate total area
  total_area = ranges::accumulate(cell_areas, 0);

  return;
}

size_t InputData::min_number_of_groups() const {
  return std::ceil(static_cast<double>(total_area) /
                   static_cast<double>(max_group_area));
}

void InputData::debug_print() const {
  fmt::print("{}\n", *this);
  fmt::print("Cell areas: {}\n", fmt::join(cell_areas, ", "));
  for (const auto& [i, net] : nets | enumerate) {
    fmt::print("Net {} contains cells: {}\n", i,
               fmt::join(net | to<std::vector>() | sort, ", "));
  }
}
