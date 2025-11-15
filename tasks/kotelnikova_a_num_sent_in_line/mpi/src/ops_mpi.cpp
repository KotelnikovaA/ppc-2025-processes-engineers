#include "kotelnikova_a_num_sent_in_line/mpi/include/ops_mpi.hpp"

#include <mpi.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>

#include "kotelnikova_a_num_sent_in_line/common/include/common.hpp"

namespace kotelnikova_a_num_sent_in_line {

KotelnikovaANumSentInLineMPI::KotelnikovaANumSentInLineMPI(const InType &in) {
  SetTypeOfTask(GetStaticTypeOfTask());
  GetInput() = in;
  GetOutput() = 0;
}

bool KotelnikovaANumSentInLineMPI::ValidationImpl() {
  return true;
}

bool KotelnikovaANumSentInLineMPI::PreProcessingImpl() {
  return true;
}

bool KotelnikovaANumSentInLineMPI::RunImpl() {
  const std::string &text = GetInput();
  int world_size = 0;
  int world_rank = 0;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

  if (world_size == 1) {
    GetOutput() = CountSentencesSequential(text);
    return true;
  }

  int total_length = static_cast<int>(text.length());
  int chunk_size = total_length / world_size;
  int remainder = total_length % world_size;

  int start = (world_rank * chunk_size) + std::min(world_rank, remainder);
  int end = start + chunk_size + (world_rank < remainder ? 1 : 0);

  int local_count = CountLocalSentences(text, start, end);
  bool has_unfinished = CheckUnfinishedContent(text, start, end);

  int global_count = SynchronizeWithNeighbors(local_count, has_unfinished, world_rank, world_size);

  DistributeResult(global_count);
  return true;
}

int KotelnikovaANumSentInLineMPI::CountLocalSentences(const std::string &text, int start, int end) {
  int local_count = 0;
  bool has_unfinished_content = false;

  for (int i = start; i < end; ++i) {
    char c = text[i];

    if (std::isalnum(static_cast<unsigned char>(c)) != 0) {
      has_unfinished_content = true;
    } else if (IsSentenceEnd(c) && has_unfinished_content) {
      local_count++;
      has_unfinished_content = false;
    }
  }

  return local_count;
}

bool KotelnikovaANumSentInLineMPI::CheckUnfinishedContent(const std::string &text, int start, int end) {
  for (int i = start; i < end; ++i) {
    if (std::isalnum(static_cast<unsigned char>(text[i])) != 0) {
      return true;
    }
  }
  return false;
}

bool KotelnikovaANumSentInLineMPI::IsSentenceEnd(char c) {
  return c == '.' || c == '!' || c == '?';
}

int KotelnikovaANumSentInLineMPI::SynchronizeWithNeighbors(int local_count, bool has_unfinished, int world_rank,
                                                           int world_size) {
  int receives_unfinished = 0;
  if (world_rank > 0) {
    MPI_Recv(&receives_unfinished, 1, MPI_INT, world_rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }

  if (receives_unfinished > 0) {
    local_count++;
  }

  int sends_unfinished = has_unfinished ? 1 : 0;
  if (world_rank < world_size - 1) {
    MPI_Send(&sends_unfinished, 1, MPI_INT, world_rank + 1, 0, MPI_COMM_WORLD);
  }

  int global_count = 0;
  MPI_Reduce(&local_count, &global_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Bcast(&global_count, 1, MPI_INT, 0, MPI_COMM_WORLD);

  return global_count;
}

void KotelnikovaANumSentInLineMPI::DistributeResult(int global_count) {
  GetOutput() = global_count;
}

int KotelnikovaANumSentInLineMPI::CountSentencesSequential(const std::string &text) {
  int count = 0;
  bool in_sentence = false;

  for (size_t i = 0; i < text.length(); ++i) {
    char c = text[i];

    if (c == '.' || c == '!' || c == '?') {
      if (in_sentence) {
        count++;
        in_sentence = false;
      }
    } else if (std::isalnum(static_cast<unsigned char>(c)) != 0) {
      in_sentence = true;
    }
  }

  if (in_sentence) {
    count++;
  }
  return count;
}

bool KotelnikovaANumSentInLineMPI::PostProcessingImpl() {
  return true;
}

}  // namespace kotelnikova_a_num_sent_in_line
