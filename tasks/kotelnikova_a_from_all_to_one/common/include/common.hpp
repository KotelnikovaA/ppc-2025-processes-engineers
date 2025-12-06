#pragma once

#include <string>
#include <tuple>

#include "task/include/task.hpp"

namespace kotelnikova_a_from_all_to_one {

using InType = int;
using OutType = int;
using TestType = std::tuple<int, std::string>;
using BaseTask = ppc::task::Task<InType, OutType>;

}  // namespace kotelnikova_a_from_all_to_one
