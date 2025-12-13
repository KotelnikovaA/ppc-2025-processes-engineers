#pragma once

#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include "task/include/task.hpp"

namespace kotelnikova_a_from_all_to_one {

using InTypeVariant = std::variant<std::vector<int>, std::vector<float>, std::vector<double>>;

using InType = InTypeVariant;
using OutType = InTypeVariant;
using TestType = std::tuple<int, std::string>;
using BaseTask = ppc::task::Task<InType, OutType>;

}  // namespace kotelnikova_a_from_all_to_one
