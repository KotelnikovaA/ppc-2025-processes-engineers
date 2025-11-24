#include <gtest/gtest.h>
#include <fstream>
#include <cstddef>
#include <string>
#include <vector>

#include "kotelnikova_a_num_sent_in_line/common/include/common.hpp"
#include "kotelnikova_a_num_sent_in_line/mpi/include/ops_mpi.hpp"
#include "kotelnikova_a_num_sent_in_line/seq/include/ops_seq.hpp"
#include "util/include/perf_test_util.hpp"

namespace kotelnikova_a_num_sent_in_line {

class KotelnikovaARunPerfTestProcesses : public ppc::util::BaseRunPerfTests<InType, OutType> {
  std::size_t expected_sentences_count_;
  InType input_data_;

  void SetUp() override {
    input_data_ = LoadTestDataFromFile();
    expected_sentences_count_ = 1;
  }

  bool CheckTestOutputData(OutType &output_data) final {
    return output_data == expected_sentences_count_;
  }

  InType GetTestInputData() final {
    return input_data_;
  }

  static std::string LoadTestDataFromFile() {
    std::vector<std::string> possible_paths = {
        "../../../tasks/kotelnikova_a_num_sent_in_line/data/test_7.txt",
        "../tasks/kotelnikova_a_num_sent_in_line/data/test_7.txt",
        "tasks/kotelnikova_a_num_sent_in_line/data/test_7.txt",
        "kotelnikova_a_num_sent_in_line/data/test_7.txt", 
        "data/test_7.txt"};

    std::ifstream file;
    for (const auto& path : possible_paths) {
      file.open(path);
      if (file.is_open()) {
        std::cout << "Loaded test data from: " << path << std::endl;
        break;
      }
    }

    std::string content;
    std::string line;
    while (std::getline(file, line)) {
      content += line + "\n";
    }
    file.close();

    if (!content.empty() && content.back() == '\n') {
      content.pop_back();
    }

    std::cout << "Loaded text length: " << content.length() << " characters" << std::endl;
    return content;
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
