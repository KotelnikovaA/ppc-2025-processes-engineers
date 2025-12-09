#include "kotelnikova_a_from_all_to_one/seq/include/ops_seq.hpp"

#include "kotelnikova_a_from_all_to_one/common/include/common.hpp"

namespace kotelnikova_a_from_all_to_one {

KotelnikovaAFromAllToOneSEQ::KotelnikovaAFromAllToOneSEQ(const InType &in) {
  SetTypeOfTask(GetStaticTypeOfTask());
  GetInput() = in;

  if (std::holds_alternative<std::vector<int>>(in)) {
    auto vec = std::get<std::vector<int>>(in);
    GetOutput() = InTypeVariant{std::vector<int>(vec)};
  } else if (std::holds_alternative<std::vector<float>>(in)) {
    auto vec = std::get<std::vector<float>>(in);
    GetOutput() = InTypeVariant{std::vector<float>(vec)};
  } else if (std::holds_alternative<std::vector<double>>(in)) {
    auto vec = std::get<std::vector<double>>(in);
    GetOutput() = InTypeVariant{std::vector<double>(vec)};
  } else {
    throw std::runtime_error("Unsupported data type");
  }
}

bool KotelnikovaAFromAllToOneSEQ::ValidationImpl() {
  auto input = GetInput();

  if (std::holds_alternative<std::vector<int>>(input)) {
    return true;
  } else if (std::holds_alternative<std::vector<float>>(input)) {
    return true;
  } else if (std::holds_alternative<std::vector<double>>(input)) {
    return true;
  }

  return false;
}

bool KotelnikovaAFromAllToOneSEQ::PreProcessingImpl() {
  return true;
}

bool KotelnikovaAFromAllToOneSEQ::RunImpl() {
  try {
    auto input = GetInput();
    auto &output = GetOutput();

    if (std::holds_alternative<std::vector<int>>(input)) {
      auto &input_vec = std::get<std::vector<int>>(input);
      auto &output_vec = std::get<std::vector<int>>(output);

      if (output_vec.size() != input_vec.size()) {
        output_vec.resize(input_vec.size());
      }

      if (!input_vec.empty()) {
        std::memcpy(output_vec.data(), input_vec.data(), input_vec.size() * sizeof(int));
      }
      return true;
    }

    if (std::holds_alternative<std::vector<float>>(input)) {
      auto &input_vec = std::get<std::vector<float>>(input);
      auto &output_vec = std::get<std::vector<float>>(output);

      if (output_vec.size() != input_vec.size()) {
        output_vec.resize(input_vec.size());
      }

      if (!input_vec.empty()) {
        std::memcpy(output_vec.data(), input_vec.data(), input_vec.size() * sizeof(float));
      }
      return true;
    }

    if (std::holds_alternative<std::vector<double>>(input)) {
      auto &input_vec = std::get<std::vector<double>>(input);
      auto &output_vec = std::get<std::vector<double>>(output);

      if (output_vec.size() != input_vec.size()) {
        output_vec.resize(input_vec.size());
      }

      if (!input_vec.empty()) {
        std::memcpy(output_vec.data(), input_vec.data(), input_vec.size() * sizeof(double));
      }
      return true;
    }

    return false;
  } catch (...) {
    return false;
  }
}

bool KotelnikovaAFromAllToOneSEQ::PostProcessingImpl() {
  return true;
}

}  // namespace kotelnikova_a_from_all_to_one
