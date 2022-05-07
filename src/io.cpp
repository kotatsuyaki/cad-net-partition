#include "io.hpp"

#include <stdexcept>

using std::istream;
using std::string;

InputData::InputData() = default;

InputData InputData::read_from(istream& is) noexcept(false) {
  InputData data;
  data.read(is);
  return data;
}

void InputData::read(istream& is) noexcept(false) {
  // enable exceptions on input errors
  is.exceptions(std::istream::failbit | std::istream::badbit);

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

  return;
}
