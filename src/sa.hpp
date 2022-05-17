#ifndef SA_HPP_
#define SA_HPP_

#include "data.hpp"

#include <vector>

std::vector<Block> perform_sa_partition(const std::vector<Block>& blocks,
                                        const InputData& inputs);

#endif  // SA_HPP_
