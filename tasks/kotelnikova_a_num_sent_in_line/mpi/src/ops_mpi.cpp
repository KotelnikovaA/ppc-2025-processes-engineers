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
  GetOutput() = static_cast<std::size_t>(0);
}

bool KotelnikovaANumSentInLineMPI::ValidationImpl() {
  if (GetInput().empty()) {
    return false;
  }
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

  // Вычисляем границы чанка
  int start = world_rank * chunk_size + std::min(world_rank, remainder);
  int end = start + chunk_size + (world_rank < remainder ? 1 : 0);
  end = std::min(end, total_length);

  // Подсчитываем предложения в чанке
  int local_count = CountSentencesInChunk(text, start, end, world_rank, world_size);

  // Суммируем результаты
  int global_count = 0;
  MPI_Allreduce(&local_count, &global_count, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

  GetOutput() = static_cast<std::size_t>(global_count);
  return true;
}

int KotelnikovaANumSentInLineMPI::CountSentencesInChunk(const std::string &text, int start, int end, int world_rank,
                                                        int world_size) {
  int count = 0;
  bool in_sentence = false;

  // Определяем начальное состояние
  if (start > 0) {
    char prev_char = text[static_cast<std::size_t>(start - 1)];
    in_sentence = !IsSentenceEnd(prev_char) && IsWordCharacter(prev_char);
  }

  for (int i = start; i < end; ++i) {
    std::size_t idx = static_cast<std::size_t>(i);
    char c = text[idx];

    if (IsSentenceEnd(c)) {
      if (in_sentence) {
        count++;
        in_sentence = false;
      }
    } else if (IsWordCharacter(c)) {
      in_sentence = true;
    }
  }

  // ВАЖНО: если чанк заканчивается на незавершенном предложении И это последний чанк,
  // то считаем это как предложение
  if (in_sentence && world_rank == world_size - 1 && end == static_cast<int>(text.length())) {
    count++;
  }

  return count;
}

bool KotelnikovaANumSentInLineMPI::IsSentenceEnd(char c) {
  return c == '.' || c == '!' || c == '?';
}

bool KotelnikovaANumSentInLineMPI::IsWordCharacter(char c) {
  unsigned char uc = static_cast<unsigned char>(c);
  return std::isalnum(uc) != 0;
}

std::size_t KotelnikovaANumSentInLineMPI::CountSentencesSequential(const std::string &text) {
  std::size_t count = 0;
  bool in_sentence = false;

  for (std::size_t i = 0; i < text.length(); ++i) {
    char c = text[i];

    if (IsSentenceEnd(c)) {
      if (in_sentence) {
        count++;
        in_sentence = false;
      }
    } else if (IsWordCharacter(c)) {
      in_sentence = true;
    }
  }

  // ВАЖНО: если текст заканчивается на незавершенном предложении, считаем его
  if (in_sentence) {
    count++;
  }

  return count;
}

bool KotelnikovaANumSentInLineMPI::PostProcessingImpl() {
  return true;
}

}  // namespace kotelnikova_a_num_sent_in_line
