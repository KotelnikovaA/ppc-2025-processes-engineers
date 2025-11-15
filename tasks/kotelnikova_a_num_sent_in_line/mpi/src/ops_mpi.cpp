#include "kotelnikova_a_num_sent_in_line/mpi/include/ops_mpi.hpp"

#include <mpi.h>

#include <algorithm>
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

  int total_length = static_cast<int>(text.length());

  if (world_size == 1) {
    GetOutput() = CountSentencesSequential(text);
    return true;
  }

  int chunk_size = total_length / world_size;
  int remainder = total_length % world_size;

  int start = (world_rank * chunk_size) + std::min(world_rank, remainder);
  int end = start + chunk_size + (world_rank < remainder ? 1 : 0);

  int local_count = 0;
  bool has_unfinished_content = false;

  for (int i = start; i < end; ++i) {
    char c = text[i];

    if (std::isalnum(static_cast<unsigned char>(c)) != 0) {
      has_unfinished_content = true;
    } else if ((c == '.' || c == '!' || c == '?') && has_unfinished_content) {
      local_count++;
      has_unfinished_content = false;
    }
  }

  int receives_unfinished = 0;
  if (world_rank > 0) {
    MPI_Recv(&receives_unfinished, 1, MPI_INT, world_rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }

  if (receives_unfinished > 0) {
    for (int i = start; i < std::min(start + 1, end); ++i) {
      char c = text[i];
      if ((c == '.' || c == '!' || c == '?') && receives_unfinished > 0) {
        local_count++;
        receives_unfinished = 0;
        break;
      }
    }
  }

  int sends_unfinished = has_unfinished_content ? 1 : 0;
  if (world_rank < world_size - 1) {
    MPI_Send(&sends_unfinished, 1, MPI_INT, world_rank + 1, 0, MPI_COMM_WORLD);
  }

  int global_count = 0;
  MPI_Reduce(&local_count, &global_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

  if (world_rank == 0) {
    GetOutput() = global_count;
  }

  MPI_Bcast(&global_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
  if (world_rank != 0) {
    GetOutput() = global_count;
  }

  return true;
}

bool KotelnikovaANumSentInLineMPI::PostProcessingImpl() {
  return true;
}

}  // namespace kotelnikova_a_num_sent_in_line
