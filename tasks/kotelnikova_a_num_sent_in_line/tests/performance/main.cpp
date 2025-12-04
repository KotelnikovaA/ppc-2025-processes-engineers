#include <gtest/gtest.h>

#include <cstddef>
#include <fstream>
#include <sstream>
#include <string>

#include "kotelnikova_a_num_sent_in_line/common/include/common.hpp"
#include "kotelnikova_a_num_sent_in_line/mpi/include/ops_mpi.hpp"
#include "kotelnikova_a_num_sent_in_line/seq/include/ops_seq.hpp"
#include "util/include/perf_test_util.hpp"

namespace kotelnikova_a_num_sent_in_line {

class KotelnikovaARunPerfTestProcesses : public ppc::util::BaseRunPerfTests<InType, OutType> {
  InType input_data_{};
  std::size_t expected_count_{};

  void SetUp() override {
    input_data_ = LoadTestData();
    expected_count_ = 1312;
  }

  bool CheckTestOutputData(OutType &output_data) final {
    return output_data == expected_count_;
  }

  InType GetTestInputData() final {
    return input_data_;
  }

  static std::string LoadTestData() {
    std::string path = "tasks/kotelnikova_a_num_sent_in_line/data/test_6.txt";
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
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
