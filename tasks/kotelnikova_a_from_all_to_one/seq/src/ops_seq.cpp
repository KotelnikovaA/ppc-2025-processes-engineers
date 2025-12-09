#include "kotelnikova_a_from_all_to_one/seq/include/ops_seq.hpp"

#include "kotelnikova_a_from_all_to_one/common/include/common.hpp"

namespace kotelnikova_a_from_all_to_one {

KotelnikovaAFromAllToOneSEQ::KotelnikovaAFromAllToOneSEQ(const InType &in) {
  SetTypeOfTask(GetStaticTypeOfTask());
  GetInput() = in;
  GetOutput() = in;
}

bool KotelnikovaAFromAllToOneSEQ::ValidationImpl() {
  return true;
}

bool KotelnikovaAFromAllToOneSEQ::PreProcessingImpl() {
  return true;
}

bool KotelnikovaAFromAllToOneSEQ::RunImpl() {
  return true;
}

bool KotelnikovaAFromAllToOneSEQ::PostProcessingImpl() {
  return true;
}

}  // namespace kotelnikova_a_from_all_to_one
