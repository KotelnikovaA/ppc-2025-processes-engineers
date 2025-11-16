#include <gtest/gtest.h>

#include <cstddef>
#include <random>
#include <string>

#include "kotelnikova_a_num_sent_in_line/common/include/common.hpp"
#include "kotelnikova_a_num_sent_in_line/mpi/include/ops_mpi.hpp"
#include "kotelnikova_a_num_sent_in_line/seq/include/ops_seq.hpp"
#include "util/include/perf_test_util.hpp"

namespace kotelnikova_a_num_sent_in_line {

class KotelnikovaARunPerfTestProcesses : public ppc::util::BaseRunPerfTests<InType, OutType> {
  const std::size_t sentences_count_ = 1000;
  InType input_data_;

  void SetUp() override {
    input_data_ = GenerateTestData(sentences_count_, 42);
  }

  bool CheckTestOutputData(OutType &output_data) final {
    return output_data == sentences_count_;
  }

  InType GetTestInputData() final {
    return input_data_;
  }

  static std::string GenerateTestData(const std::size_t sentences_count, const int seed) {
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> word_len_dist(3, 15);
    std::uniform_int_distribution<int> words_in_sentence_dist(3, 12);
    std::uniform_int_distribution<int> char_dist('A', 'z');
    std::string result;
    result.reserve(sentences_count * 100);

    for (std::size_t i = 0; i < sentences_count; ++i) {
      int words_count = words_in_sentence_dist(gen);

      for (int w = 0; w < words_count; ++w) {
        int word_length = word_len_dist(gen);
        for (int j = 0; j < word_length; ++j) {
          result += static_cast<char>(char_dist(gen));
        }

        if (w < words_count - 1) {
          result += ' ';
        }
      }

      const char sentence_enders[] = {'.', '!', '?'};
      std::uniform_int_distribution<int> ender_dist(0, 2);
      result += sentence_enders[ender_dist(gen)];

      if (i < sentences_count - 1) {
        result += ' ';
      }
    }

    return result;
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
