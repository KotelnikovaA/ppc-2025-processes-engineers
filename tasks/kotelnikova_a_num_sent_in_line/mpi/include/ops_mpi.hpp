#pragma once

#include <string>
#include <vector>

#include "kotelnikova_a_num_sent_in_line/common/include/common.hpp"
#include "task/include/task.hpp"

namespace kotelnikova_a_num_sent_in_line {

class KotelnikovaANumSentInLineMPI : public BaseTask {
 public:
  static constexpr ppc::task::TypeOfTask GetStaticTypeOfTask() {
    return ppc::task::TypeOfTask::kMPI;
  }
  explicit KotelnikovaANumSentInLineMPI(const InType &in);

 private:
  bool ValidationImpl() override;
  bool PreProcessingImpl() override;
  bool RunImpl() override;
  bool PostProcessingImpl() override;

  static int CountLocalSentences(const std::string &text, int start, int end, bool &has_unfinished);
  static bool CheckSentenceStateAtStart(const std::string &text, int start);
  static bool CheckUnfinishedAtBoundary(const std::string &text, int position, int total_length);
  static bool ScanForPunctuation(const std::string &text, int start, int total_length);
  static int CalculateGlobalCount(const std::vector<int> &all_counts, const std::vector<int> &all_unfinished,
                                  const std::string &text, int chunk_size, int remainder, int total_length,
                                  int world_size);
};

}  // namespace kotelnikova_a_num_sent_in_line
