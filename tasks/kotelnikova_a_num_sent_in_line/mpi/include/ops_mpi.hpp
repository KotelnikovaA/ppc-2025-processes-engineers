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

  int count_sentences_sequential(const std::string &text) {
    int count = 0;
    bool in_sentence = false;

    for (size_t i = 0; i < text.length(); ++i) {
      char c = text[i];

      if (c == '.' || c == '!' || c == '?') {
        if (in_sentence) {
          count++;
          in_sentence = false;
        }
      } else if (std::isalnum(static_cast<unsigned char>(c))) {
        in_sentence = true;
      }
    }

    if (in_sentence) {
      count++;
    }
    return count;
  }

  int correct_overlap_counts(const std::string &text, int total_count, int chunk_size, int num_processes, int overlap) {
    int overlap_correction = 0;

    // Подсчитываем предложения в зонах перекрытия между процессами
    for (int i = 1; i < num_processes; i++) {
      int overlap_start = i * chunk_size;
      int overlap_end = std::min(overlap_start + overlap, static_cast<int>(text.length()));

      if (overlap_start < static_cast<int>(text.length())) {
        // Подсчитываем предложения в этой overlap-зоне
        int overlap_count = 0;
        bool in_sentence = false;

        for (int j = overlap_start; j < overlap_end && j < static_cast<int>(text.length()); ++j) {
          char c = text[j];

          if (c == '.' || c == '!' || c == '?') {
            if (in_sentence) {
              overlap_count++;
              in_sentence = false;
            }
          } else if (std::isalnum(static_cast<unsigned char>(c))) {
            in_sentence = true;
          }
        }

        // Учитываем предложение без знака препинания в конце overlap-зоны
        if (in_sentence) {
          overlap_count++;
        }

        overlap_correction += overlap_count;
      }
    }

    return total_count - overlap_correction;
  }
};

}  // namespace kotelnikova_a_num_sent_in_line
