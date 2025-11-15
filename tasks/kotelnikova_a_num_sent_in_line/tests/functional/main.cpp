#include <gtest/gtest.h>

#include <array>
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
    int expected = std::get<1>(test_param);

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
  int expected_count_ = 0;
};

namespace {

TEST_P(KotelnikovaARunFuncTestsProcesses, MatmulFromPic) {
  ExecuteTest(GetParam());
}

const std::array<TestType, 5> kTestParam = {
    std::make_tuple("Hello world.", 1),           std::make_tuple("First. Second.", 2),
    std::make_tuple("How are you? I'm fine!", 2), std::make_tuple("What? When? Where?", 3),
    std::make_tuple("Just one sentence.", 1),     std::make_tuple("Just. One. Sentence.", 3)};

const auto kTestTasksList = std::tuple_cat(ppc::util::AddFuncTask<KotelnikovaANumSentInLineMPI, InType>(
                                               kTestParam, PPC_SETTINGS_kotelnikova_a_num_sent_in_line),
                                           ppc::util::AddFuncTask<KotelnikovaANumSentInLineSEQ, InType>(
                                               kTestParam, PPC_SETTINGS_kotelnikova_a_num_sent_in_line));

const auto kGtestValues = ppc::util::ExpandToValues(kTestTasksList);

const auto kPerfTestName = KotelnikovaARunFuncTestsProcesses::PrintFuncTestName<KotelnikovaARunFuncTestsProcesses>;

INSTANTIATE_TEST_SUITE_P(SentenceCountingTests, KotelnikovaARunFuncTestsProcesses, kGtestValues, kPerfTestName);
}  // namespace

}  // namespace kotelnikova_a_num_sent_in_line
