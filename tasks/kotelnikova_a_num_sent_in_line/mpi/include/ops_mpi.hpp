#pragma once

#include <cctype>
#include <string>

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

  static int CountSentencesSequential(const std::string &text);
  static int CountLocalSentences(const std::string &text, int start, int end);
  static bool CheckUnfinishedContent(const std::string &text, int start, int end);
  static bool IsSentenceEnd(char c);
  static int SynchronizeWithNeighbors(int local_count, bool has_unfinished, int world_rank, int world_size);
  void DistributeResult(int global_count);
};

}  // namespace kotelnikova_a_num_sent_in_line
