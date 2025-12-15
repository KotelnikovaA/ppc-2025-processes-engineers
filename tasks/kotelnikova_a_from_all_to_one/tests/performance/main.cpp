#include <gtest/gtest.h>
#include <mpi.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <tuple>
#include <vector>

#include "kotelnikova_a_from_all_to_one/common/include/common.hpp"
#include "kotelnikova_a_from_all_to_one/mpi/include/ops_mpi.hpp"
#include "kotelnikova_a_from_all_to_one/seq/include/ops_seq.hpp"
#include "util/include/perf_test_util.hpp"

// комментарий для пуша так как код упал так еще и не по моей вине

namespace kotelnikova_a_from_all_to_one {

class KotelnikovaARunPerfTestProcesses2 : public ppc::util::BaseRunPerfTests<InType, OutType> {
 protected:
  void SetUp() override {
    auto param = GetParam();
    std::string task_name = std::get<1>(param);
    is_mpi_test_ = (task_name.find("mpi") != std::string::npos);

    size_t size = 4000000;
    std::vector<double> data(size);

    for (size_t i = 0; i < size; i++) {
      data[i] = static_cast<double>(i % 1000) - 500.0;
    }

    input_data_ = InType{data};
  }

  bool CheckTestOutputData(InType &output_data) final {
    try {
      int rank = 0;
      int mpi_size = 0;
      MPI_Comm_rank(MPI_COMM_WORLD, &rank);
      MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

      auto &input_vec = std::get<std::vector<double>>(input_data_);

      if (!is_mpi_test_) {
        auto &output_vec = std::get<std::vector<double>>(output_data);

        if (output_vec.size() != input_vec.size()) {
          return false;
        }

        for (size_t i = 0; i < std::min<size_t>(output_vec.size(), 10); i++) {
          if (std::abs(output_vec[i] - input_vec[i]) > 1e-9) {
            return false;
          }
        }

        return true;
      }

      if (rank == 0) {
        auto &output_vec = std::get<std::vector<double>>(output_data);

        if (output_vec.empty() || output_vec.size() != input_vec.size()) {
          return false;
        }

        for (size_t i = 0; i < std::min<size_t>(output_vec.size(), 10); i++) {
          double expected_val = input_vec[i] * mpi_size;
          double diff = std::abs(output_vec[i] - expected_val);

          if (diff > 1e-6) {
            return false;
          }
        }

        return true;
      }
      return true;
    } catch (...) {
      return false;
    }
  }

  InTypeVariant GetTestInputData() final {
    return input_data_;
  }

  [[nodiscard]] bool IsMpiTest() const {
    return is_mpi_test_;
  }

 private:
  InTypeVariant input_data_;
  bool is_mpi_test_ = false;
};

namespace {

TEST_P(KotelnikovaARunPerfTestProcesses2, RunPerfModes) {
  ExecuteTest(GetParam());
}

const auto kAllPerfTasks =
    ppc::util::MakeAllPerfTasks<InType, KotelnikovaAFromAllToOneSEQ, KotelnikovaAFromAllToOneMPI>(
        PPC_SETTINGS_kotelnikova_a_from_all_to_one);

const auto kGtestValues = ppc::util::TupleToGTestValues(kAllPerfTasks);

const auto kPerfTestName = KotelnikovaARunPerfTestProcesses2::CustomPerfTestName;

INSTANTIATE_TEST_SUITE_P(RunModeTests, KotelnikovaARunPerfTestProcesses2, kGtestValues, kPerfTestName);

}  // namespace

}  // namespace kotelnikova_a_from_all_to_one
