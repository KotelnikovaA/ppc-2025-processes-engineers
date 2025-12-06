#include <gtest/gtest.h>

#include "kotelnikova_a_from_all_to_one/common/include/common.hpp"
#include "kotelnikova_a_from_all_to_one/mpi/include/ops_mpi.hpp"
#include "kotelnikova_a_from_all_to_one/seq/include/ops_seq.hpp"
#include "util/include/perf_test_util.hpp"

namespace kotelnikova_a_from_all_to_one {

class KotelnikovaARunPerfTestProcesses2 : public ppc::util::BaseRunPerfTests<InType, OutType> {
  const int kCount_ = 100;
  InType input_data_{};

  void SetUp() override {
    input_data_ = kCount_;
  }

  bool CheckTestOutputData(OutType &output_data) final {
    return input_data_ == output_data;
  }

  InType GetTestInputData() final {
    return input_data_;
  }
};

TEST_P(KotelnikovaARunPerfTestProcesses2, RunPerfModes) {
  ExecuteTest(GetParam());
}

const auto kAllPerfTasks =
    ppc::util::MakeAllPerfTasks<InType, KotelnikovaAFromAllToOneMPI, KotelnikovaAFromAllToOneSEQ>(PPC_SETTINGS_kotelnikova_a_from_all_to_one);

const auto kGtestValues = ppc::util::TupleToGTestValues(kAllPerfTasks);

const auto kPerfTestName = KotelnikovaARunPerfTestProcesses2::CustomPerfTestName;

INSTANTIATE_TEST_SUITE_P(RunModeTests, KotelnikovaARunPerfTestProcesses2, kGtestValues, kPerfTestName);

}  // namespace kotelnikova_a_from_all_to_one
