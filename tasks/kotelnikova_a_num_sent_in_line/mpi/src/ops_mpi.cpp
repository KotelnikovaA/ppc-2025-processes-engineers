#include "kotelnikova_a_num_sent_in_line/mpi/include/ops_mpi.hpp"

#include <mpi.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <vector>

#include "kotelnikova_a_num_sent_in_line/common/include/common.hpp"

namespace kotelnikova_a_num_sent_in_line {

KotelnikovaANumSentInLineMPI::KotelnikovaANumSentInLineMPI(const InType &in) {
  SetTypeOfTask(GetStaticTypeOfTask());
  GetInput() = in;
  GetOutput() = static_cast<std::size_t>(0);
}

bool KotelnikovaANumSentInLineMPI::ValidationImpl() {
  return !GetInput().empty();
}

bool KotelnikovaANumSentInLineMPI::PreProcessingImpl() {
  return true;
}

bool KotelnikovaANumSentInLineMPI::RunImpl() {
  int world_size = 0;
  int world_rank = 0;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

  int total_length = 0;
  if (world_rank == 0) {
    total_length = static_cast<int>(GetInput().length());
  }
  MPI_Bcast(&total_length, 1, MPI_INT, 0, MPI_COMM_WORLD);

  std::string text;
  if (world_rank == 0) {
    text = GetInput();
  } else {
    text.resize(static_cast<std::size_t>(total_length));
  }
  MPI_Bcast(text.data(), total_length, MPI_CHAR, 0, MPI_COMM_WORLD);

  int chunk_size = total_length / world_size;
  int remainder = total_length % world_size;
  int start = (world_rank * chunk_size) + std::min(world_rank, remainder);
  int end = start + chunk_size + (world_rank < remainder ? 1 : 0);
  end = std::min(end, total_length);

  int local_count = 0;
  bool has_unfinished = false;

  if (start < end) {
    local_count = CountLocalSentences(text, start, end, has_unfinished);
  }

  std::vector<int> all_counts(static_cast<std::size_t>(world_size));
  MPI_Allgather(&local_count, 1, MPI_INT, all_counts.data(), 1, MPI_INT, MPI_COMM_WORLD);

  int unfinished_int = has_unfinished ? 1 : 0;
  std::vector<int> all_unfinished(static_cast<std::size_t>(world_size));
  MPI_Allgather(&unfinished_int, 1, MPI_INT, all_unfinished.data(), 1, MPI_INT, MPI_COMM_WORLD);

  int global_count =
      CalculateGlobalCount(all_counts, all_unfinished, text, chunk_size, remainder, total_length, world_size);

  GetOutput() = static_cast<std::size_t>(global_count);
  return true;
}

int KotelnikovaANumSentInLineMPI::CountLocalSentences(const std::string &text, int start, int end,
                                                      bool &has_unfinished) {
  int local_count = 0;
  bool in_sentence = false;

  in_sentence = CheckSentenceStateAtStart(text, start);

  for (int i = start; i < end; ++i) {
    char c = text[static_cast<std::size_t>(i)];

    if (c == '.' || c == '!' || c == '?') {
      if (in_sentence) {
        local_count++;
        in_sentence = false;
      }
    } else if (std::isalnum(static_cast<unsigned char>(c)) != 0) {
      in_sentence = true;
    }
  }

  has_unfinished = in_sentence;
  return local_count;
}

bool KotelnikovaANumSentInLineMPI::CheckSentenceStateAtStart(const std::string &text, int start) {
  if (start == 0) {
    return false;
  }

  for (int i = start - 1; i >= 0; --i) {
    char c = text[static_cast<std::size_t>(i)];

    if (c == '.' || c == '!' || c == '?') {
      return false;
    }

    if (std::isalnum(static_cast<unsigned char>(c)) != 0) {
      return true;
    }
  }

  return false;
}

int KotelnikovaANumSentInLineMPI::CalculateGlobalCount(const std::vector<int> &all_counts,
                                                       const std::vector<int> &all_unfinished, const std::string &text,
                                                       int chunk_size, int remainder, int total_length,
                                                       int world_size) {
  int global_count = 0;

  for (int count : all_counts) {
    global_count += count;
  }

  for (int i = 0; i < world_size - 1; ++i) {
    if (all_unfinished[i] == 0) {
      continue;
    }

    int next_start = ((i + 1) * chunk_size) + std::min(i + 1, remainder);
    if (next_start >= total_length) {
      continue;
    }

    bool found_punctuation = false;
    for (int j = next_start; j < total_length; ++j) {
      char c = text[static_cast<std::size_t>(j)];

      if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
        continue;
      }

      if (std::isalnum(static_cast<unsigned char>(c)) != 0) {
        break;
      }

      if (c == '.' || c == '!' || c == '?') {
        found_punctuation = true;
        break;
      }
    }

    if (found_punctuation) {
      global_count++;
    }
  }

  if (world_size > 0 && all_unfinished[world_size - 1] == 1) {
    global_count++;
  }

  return global_count;
}

bool KotelnikovaANumSentInLineMPI::PostProcessingImpl() {
  return true;
}

}  // namespace kotelnikova_a_num_sent_in_line
