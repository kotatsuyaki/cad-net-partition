#ifndef DATA_HPP_
#define DATA_HPP_

#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <parallel_hashmap/phmap.h>

#include <istream>
#include <vector>

using Net = phmap::flat_hash_set<size_t>;
using Cell = phmap::flat_hash_set<size_t>;

struct InputData {
  static InputData read_from(std::istream& is) noexcept(false);
  void read(std::istream& is) noexcept(false);
  size_t min_number_of_groups() const;
  void debug_print() const;

  size_t max_group_area = 0;
  size_t ncells = 0;
  size_t nnets = 0;

  // Area of cells, indexed by cell numbers
  std::vector<size_t> cell_areas;

  // Hyperedges, indexed by net numbers
  std::vector<Net> nets;

  // Mapping from cell index to net indices
  std::vector<Cell> cells;

  size_t total_area = 0;
};

#include <fmt/format.h>

struct point {
  double x, y;
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
                     "InputData({} cells, {} nets, max group area {})",
                     inputs.ncells, inputs.nnets, inputs.max_group_area);
  }
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
