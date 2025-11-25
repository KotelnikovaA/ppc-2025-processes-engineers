#include <gtest/gtest.h>

#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "kotelnikova_a_num_sent_in_line/common/include/common.hpp"
#include "kotelnikova_a_num_sent_in_line/mpi/include/ops_mpi.hpp"
#include "kotelnikova_a_num_sent_in_line/seq/include/ops_seq.hpp"
#include "util/include/perf_test_util.hpp"

namespace kotelnikova_a_num_sent_in_line {

class KotelnikovaARunPerfTestProcesses : public ppc::util::BaseRunPerfTests<InType, OutType> {
  std::size_t expected_sentences_count_ = 1;
  InType input_data_;

  void SetUp() override {
    input_data_ = LoadTestDataFromFile();
  }

  bool CheckTestOutputData(OutType &output_data) final {
    return output_data == expected_sentences_count_;
  }

  InType GetTestInputData() final {
    return input_data_;
  }

  static std::string LoadTestDataFromFile() {
    std::vector<std::string> possible_paths = {"../../../tasks/kotelnikova_a_num_sent_in_line/data/test_7.txt",
                                               "../tasks/kotelnikova_a_num_sent_in_line/data/test_7.txt",
                                               "tasks/kotelnikova_a_num_sent_in_line/data/test_7.txt",
                                               "kotelnikova_a_num_sent_in_line/data/test_7.txt", "data/test_7.txt"};

    std::ifstream file;
    std::string content;
    std::string line;
    while (std::getline(file, line)) {
      content += line + "\n";
    }
    file.close();

    if (!content.empty() && content.back() == '\n') {
      content.pop_back();
    }

    return content;
  }
};

TEST_P(KotelnikovaARunPerfTestProcesses, RunPerfModes) {
  ExecuteTest(GetParam());
}

namespace {

const auto kAllPerfTasks =
    ppc::util::MakeAllPerfTasks<InType, KotelnikovaANumSentInLineMPI, KotelnikovaANumSentInLineSEQ>(
        PPC_SETTINGS_kotelnikova_a_num_sent_in_line);

const auto kGtestValues = ppc::util::TupleToGTestValues(kAllPerfTasks);

const auto kPerfTestName = KotelnikovaARunPerfTestProcesses::CustomPerfTestName;

INSTANTIATE_TEST_SUITE_P(RunModeTests, KotelnikovaARunPerfTestProcesses, kGtestValues, kPerfTestName);

}  // namespace

}  // namespace kotelnikova_a_num_sent_in_line
