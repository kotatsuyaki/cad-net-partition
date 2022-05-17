#ifndef SA_HPP_
#define SA_HPP_

#include "data.hpp"

#include <vector>

using Cost = std::int64_t;

std::vector<Block> perform_sa_partition(const std::vector<Block>& blocks,
                                        const InputData& inputs,
                                        Cost init_cost);

#endif  // SA_HPP_
