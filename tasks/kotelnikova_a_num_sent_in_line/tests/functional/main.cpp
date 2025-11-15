#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <string>
#include <tuple>

#include "kotelnikova_a_num_sent_in_line/common/include/common.hpp"
#include "kotelnikova_a_num_sent_in_line/mpi/include/ops_mpi.hpp"
#include "kotelnikova_a_num_sent_in_line/seq/include/ops_seq.hpp"
#include "util/include/func_test_util.hpp"
#include "util/include/util.hpp"

namespace kotelnikova_a_num_sent_in_line {

class KotelnikovaARunFuncTestsProcesses : public ppc::util::BaseRunFuncTests<InType, OutType, TestType> {
 public:
  static std::string PrintTestParam(const TestType &test_param) {
    static int test_counter = 0;
    test_counter++;

    const std::string &text = std::get<0>(test_param);
    std::size_t expected = std::get<1>(test_param);

    std::string name = "test_" + std::to_string(test_counter);
    name += "_len" + std::to_string(text.length());
    name += "_exp" + std::to_string(expected);
    return name;
  }

 protected:
  void SetUp() override {
    TestType params = std::get<static_cast<std::size_t>(ppc::util::GTestParamIndex::kTestParams)>(GetParam());
    input_data_ = std::get<0>(params);
    expected_count_ = std::get<1>(params);
  }

  bool CheckTestOutputData(OutType &output_data) final {
    return (expected_count_ == output_data);
  }

  InType GetTestInputData() final {
    return input_data_;
  }

 private:
  InType input_data_;
  std::size_t expected_count_ = 0;
};

namespace {

TEST_P(KotelnikovaARunFuncTestsProcesses, SentenceCountingTests) {
  ExecuteTest(GetParam());
}

std::array<TestType, 6> LoadTestData() {
  std::array<TestType, 6> test_cases;
  
  std::array<std::pair<std::string, std::size_t>, 6> test_files = {{
    {"test_1.txt", 1},
    {"test_2.txt", 3}, 
    {"test_3.txt", 8},
    {"test_4.txt", 11},
    {"test_5.txt", 4},
    {"test_6.txt", 41}
  }};
  
  for (size_t i = 0; i < test_files.size(); ++i) {
    const auto& file_info = test_files[i];
    
    std::vector<std::string> possible_paths = {
      "../../../tasks/kotelnikova_a_num_sent_in_line/data/" + file_info.first,
      "../tasks/kotelnikova_a_num_sent_in_line/data/" + file_info.first,
      "tasks/kotelnikova_a_num_sent_in_line/data/" + file_info.first,
      "kotelnikova_a_num_sent_in_line/data/" + file_info.first,
      "data/" + file_info.first
    };
    
    bool file_loaded = false;
    for (const auto& path : possible_paths) {
      std::ifstream file(path);
      if (file.is_open()) {
        std::stringstream ss;
        ss << file.rdbuf();
        std::string content = ss.str();
        test_cases[i] = std::make_tuple(content, file_info.second);
        file_loaded = true;
        std::cout << "Successfully loaded: " << path << std::endl;
        break;
      }
    }
    
    if (!file_loaded) {
      std::cout << "WARNING: Using fallback data for " << file_info.first << std::endl;
      std::string fallback_text = "Fallback text for " + file_info.first + ".";
      test_cases[i] = std::make_tuple(fallback_text, file_info.second);
    }
  }
  
  return test_cases;
}

const std::array<TestType, 6> kTestParam = LoadTestData();

const auto kTestTasksList = std::tuple_cat(
    ppc::util::AddFuncTask<KotelnikovaANumSentInLineMPI, InType>(
        kTestParam, PPC_SETTINGS_kotelnikova_a_num_sent_in_line),
    ppc::util::AddFuncTask<KotelnikovaANumSentInLineSEQ, InType>(
        kTestParam, PPC_SETTINGS_kotelnikova_a_num_sent_in_line)
);

const auto kGtestValues = ppc::util::ExpandToValues(kTestTasksList);

const auto kPerfTestName = KotelnikovaARunFuncTestsProcesses::PrintFuncTestName<KotelnikovaARunFuncTestsProcesses>;

INSTANTIATE_TEST_SUITE_P(SentenceCountingTests, KotelnikovaARunFuncTestsProcesses, kGtestValues, kPerfTestName);

}  // namespace

}  // namespace kotelnikova_a_num_sent_in_line