#ifndef PARTITION_HPP_
#define PARTITION_HPP_

#include "data.hpp"

#include <vector>

std::vector<Block> perform_pass(const std::vector<Block>& blocks,
                                const InputData& inputs);

#endif  // PARTITION_HPP_
