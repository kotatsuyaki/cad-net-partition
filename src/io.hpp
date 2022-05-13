#ifndef IO_HPP_
#define IO_HPP_

#include <parallel_hashmap/phmap.h>

#include <istream>
#include <vector>

using Net = phmap::flat_hash_set<size_t>;

class InputData {
 public:
  static InputData read_from(std::istream& is) noexcept(false);

  size_t max_block_area = 0;
  size_t ncells = 0;
  size_t nnets = 0;

  std::vector<size_t> cell_areas;
  std::vector<Net> nets;

 private:
  InputData();
  void read(std::istream& is) noexcept(false);
};

#endif  // IO_HPP_
