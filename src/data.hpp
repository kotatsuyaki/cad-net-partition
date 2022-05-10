#ifndef DATA_HPP_
#define DATA_HPP_

#include <parallel_hashmap/phmap.h>

#include <istream>
#include <vector>

using Net = phmap::flat_hash_set<size_t>;

struct InputData {
  static InputData read_from(std::istream& is) noexcept(false);
  void read(std::istream& is) noexcept(false);
  size_t min_number_of_groups() const;

  size_t max_group_area = 0;
  size_t ncells = 0;
  size_t nnets = 0;

  // Area of cells, indexed by cell numbers
  std::vector<size_t> cell_areas;

  // Hyperedges, indexed by net numbers
  std::vector<Net> nets;

  size_t total_area = 0;
};

struct Group {
  // Total area of the group
  size_t area = 0;

  // Cells contained in this group
  std::vector<size_t> cells;
};

inline bool operator<(const Group& ga, const Group& gb) {
  return ga.area <= gb.area;
}
inline bool operator>(const Group& ga, const Group& gb) {
  return ga.area > gb.area;
}
inline bool operator<=(const Group& ga, const Group& gb) {
  return ga.area <= gb.area;
}
inline bool operator>=(const Group& ga, const Group& gb) {
  return ga.area >= gb.area;
}
inline bool operator==(const Group& ga, const Group& gb) {
  return ga.area == gb.area;
}
inline bool operator!=(const Group& ga, const Group& gb) {
  return ga.area != gb.area;
}

#endif  // DATA_HPP_
