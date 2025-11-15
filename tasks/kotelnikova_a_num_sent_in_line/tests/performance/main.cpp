#include <gtest/gtest.h>

#include <string>

#include "kotelnikova_a_num_sent_in_line/common/include/common.hpp"
#include "kotelnikova_a_num_sent_in_line/mpi/include/ops_mpi.hpp"
#include "kotelnikova_a_num_sent_in_line/seq/include/ops_seq.hpp"
#include "util/include/perf_test_util.hpp"

namespace kotelnikova_a_num_sent_in_line {

class KotelnikovaARunPerfTestProcesses : public ppc::util::BaseRunPerfTests<InType, OutType> {
  InType input_data_{};

  void SetUp() override {
    input_data_ =
        "This is a test sentence. How are you today? "
        "I hope everything is going well! Another sentence. "
        "Simple text for performance testing. Hello world. "
        "This is a test sentence. How are you today? "
        "I hope everything is going well! Another sentence. "
        "Simple text for performance testing. Hello world. "
        "This is a test sentence. How are you today? "
        "I hope everything is going well! Another sentence. "
        "Simple text for performance testing. Hello world.";
  }

  bool CheckTestOutputData(OutType &output_data) final {
    return output_data >= 0 && output_data <= static_cast<int>(input_data_.length());
  }

  InType GetTestInputData() final {
    return input_data_;
  }
};

TEST_P(KotelnikovaARunPerfTestProcesses, RunPerfModes) {
  ExecuteTest(GetParam());
}

const auto kAllPerfTasks =
    ppc::util::MakeAllPerfTasks<InType, KotelnikovaANumSentInLineMPI, KotelnikovaANumSentInLineSEQ>(
        PPC_SETTINGS_kotelnikova_a_num_sent_in_line);

const auto kGtestValues = ppc::util::TupleToGTestValues(kAllPerfTasks);

const auto kPerfTestName = KotelnikovaARunPerfTestProcesses::CustomPerfTestName;

INSTANTIATE_TEST_SUITE_P(RunModeTests, KotelnikovaARunPerfTestProcesses, kGtestValues, kPerfTestName);

}  // namespace kotelnikova_a_num_sent_in_line
