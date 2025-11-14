#pragma once

#include <string>
#include <tuple>

#include "task/include/task.hpp"

namespace kotelnikova_a_num_sent_in_line {

using InType = std::string;
using OutType = int;
using TestType = std::tuple<std::string, int>;
using BaseTask = ppc::task::Task<InType, OutType>;

}  // namespace kotelnikova_a_num_sent_in_line
