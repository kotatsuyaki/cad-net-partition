#ifndef COST_HPP_
#define COST_HPP_

#include "data.hpp"

#include <vector>

using Cost = int64_t;

Cost find_cost(const std::vector<Block>& blocks, const InputData& inputs);

#endif  // COST_HPP_
