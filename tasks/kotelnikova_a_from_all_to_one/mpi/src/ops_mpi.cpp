#include "kotelnikova_a_from_all_to_one/mpi/include/ops_mpi.hpp"

#include <mpi.h>

#include <cstring>
#include <stdexcept>
#include <variant>
#include <vector>

#include "kotelnikova_a_from_all_to_one/common/include/common.hpp"

namespace kotelnikova_a_from_all_to_one {

KotelnikovaAFromAllToOneMPI::KotelnikovaAFromAllToOneMPI(const InType &in) {
  SetTypeOfTask(GetStaticTypeOfTask());
  GetInput() = in;

  int mpi_initialized = 0;
  MPI_Initialized(&mpi_initialized);

  if (mpi_initialized) {
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Создаем выходной буфер только на root процессе
    if (rank == 0) {
      if (std::holds_alternative<std::vector<int>>(in)) {
        auto vec = std::get<std::vector<int>>(in);
        GetOutput() = InTypeVariant{std::vector<int>(vec.size(), 0)};
      } else if (std::holds_alternative<std::vector<float>>(in)) {
        auto vec = std::get<std::vector<float>>(in);
        GetOutput() = InTypeVariant{std::vector<float>(vec.size(), 0.0F)};
      } else if (std::holds_alternative<std::vector<double>>(in)) {
        auto vec = std::get<std::vector<double>>(in);
        GetOutput() = InTypeVariant{std::vector<double>(vec.size(), 0.0)};
      }
    } else {
      GetOutput() = InTypeVariant{};
    }
  }
}

bool KotelnikovaAFromAllToOneMPI::ValidationImpl() {
  return true;
}

bool KotelnikovaAFromAllToOneMPI::PreProcessingImpl() {
  return true;
}

bool KotelnikovaAFromAllToOneMPI::RunImpl() {
  try {
    auto input = GetInput();
    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (std::holds_alternative<std::vector<int>>(input)) {
      auto original_data = std::get<std::vector<int>>(input);
      int count = static_cast<int>(original_data.size());

      if (rank == 0) {
        auto &output_variant = GetOutput();
        auto &result_data = std::get<std::vector<int>>(output_variant);
        std::memcpy(result_data.data(), original_data.data(), count * sizeof(int));
        std::vector<int> recv_buf(count);
        for (int src = 1; src < size; src++) {
          MPI_Recv(recv_buf.data(), count, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          for (int i = 0; i < count; i++) {
            result_data[i] += recv_buf[i];
          }
        }
      } else {
        MPI_Send(original_data.data(), count, MPI_INT, 0, 0, MPI_COMM_WORLD);
      }
      return true;
    }

    if (std::holds_alternative<std::vector<float>>(input)) {
      auto original_data = std::get<std::vector<float>>(input);
      int count = static_cast<int>(original_data.size());

      if (rank == 0) {
        auto &output_variant = GetOutput();
        auto &result_data = std::get<std::vector<float>>(output_variant);

        std::memcpy(result_data.data(), original_data.data(), count * sizeof(float));

        std::vector<float> recv_buf(count);
        for (int src = 1; src < size; src++) {
          MPI_Recv(recv_buf.data(), count, MPI_FLOAT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          for (int i = 0; i < count; i++) {
            result_data[i] += recv_buf[i];
          }
        }
      } else {
        MPI_Send(original_data.data(), count, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
      }
      return true;
    }

    if (std::holds_alternative<std::vector<double>>(input)) {
      auto original_data = std::get<std::vector<double>>(input);
      int count = static_cast<int>(original_data.size());

      if (rank == 0) {
        auto &output_variant = GetOutput();
        auto &result_data = std::get<std::vector<double>>(output_variant);

        std::memcpy(result_data.data(), original_data.data(), count * sizeof(double));

        std::vector<double> recv_buf(count);
        for (int src = 1; src < size; src++) {
          MPI_Recv(recv_buf.data(), count, MPI_DOUBLE, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          for (int i = 0; i < count; i++) {
            result_data[i] += recv_buf[i];
          }
        }
      } else {
        MPI_Send(original_data.data(), count, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
      }
      return true;
    }

    return false;
  } catch (...) {
    return false;
  }
}

bool KotelnikovaAFromAllToOneMPI::PostProcessingImpl() {
  return true;
}

}  // namespace kotelnikova_a_from_all_to_one
