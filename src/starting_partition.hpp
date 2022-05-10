#ifndef STARTING_PARTITION_HPP_
#define STARTING_PARTITION_HPP_

#include "data.hpp"

#include <vector>

// Finds an starting partition with unspecified (but legal) #groups.
// Throws if it is impossible to partition.
std::vector<Group> find_starting_partition(const InputData& inputs) noexcept(
    false);

#endif  // STARTING_PARTITION_HPP_
