#pragma once

#include <algorithm>
#include <cctype>
#include <iostream>
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
};

}  // namespace kotelnikova_a_num_sent_in_line
