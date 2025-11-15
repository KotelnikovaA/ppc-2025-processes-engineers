#pragma once

#include <cctype>
#include <cstddef>
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
  int CountLocalSentences(const std::string &text, int start, int end);
  bool CheckUnfinishedContent(const std::string &text, int start, int end);
  bool IsSentenceEnd(char c);
  int SynchronizeWithNeighbors(int local_count, bool has_unfinished, int world_rank, int world_size);
  void DistributeResult(int global_count);
};

}  // namespace kotelnikova_a_num_sent_in_line
