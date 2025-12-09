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

  int rank = 0;
  int mpi_initialized = 0;
  MPI_Initialized(&mpi_initialized);

  if (mpi_initialized != 0) {
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  }

  if (std::holds_alternative<std::vector<int>>(in)) {
    auto vec = std::get<std::vector<int>>(in);
    GetOutput() = InTypeVariant{std::vector<int>(vec.size(), 0)};
  } else if (std::holds_alternative<std::vector<float>>(in)) {
    auto vec = std::get<std::vector<float>>(in);
    GetOutput() = InTypeVariant{std::vector<float>(vec.size(), 0.0F)};
  } else if (std::holds_alternative<std::vector<double>>(in)) {
    auto vec = std::get<std::vector<double>>(in);
    GetOutput() = InTypeVariant{std::vector<double>(vec.size(), 0.0)};
  } else {
    throw std::runtime_error("Unsupported data type");
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
    int root = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (std::holds_alternative<std::vector<int>>(input)) {
      auto original_data = std::get<std::vector<int>>(input);

      if (rank == root) {
        auto &output_variant = GetOutput();
        auto &result_data = std::get<std::vector<int>>(output_variant);

        CustomReduce(original_data.data(), result_data.data(), static_cast<int>(original_data.size()), MPI_INT, MPI_SUM,
                     MPI_COMM_WORLD, root);

      } else {
        std::vector<int> temp_result(original_data.size());
        CustomReduce(original_data.data(), temp_result.data(), static_cast<int>(original_data.size()), MPI_INT, MPI_SUM,
                     MPI_COMM_WORLD, root);
      }
      return true;
    }

    if (std::holds_alternative<std::vector<float>>(input)) {
      auto original_data = std::get<std::vector<float>>(input);

      if (rank == root) {
        auto &output_variant = GetOutput();
        auto &result_data = std::get<std::vector<float>>(output_variant);

        CustomReduce(original_data.data(), result_data.data(), static_cast<int>(original_data.size()), MPI_FLOAT,
                     MPI_SUM, MPI_COMM_WORLD, root);

      } else {
        std::vector<float> temp_result(original_data.size());
        CustomReduce(original_data.data(), temp_result.data(), static_cast<int>(original_data.size()), MPI_FLOAT,
                     MPI_SUM, MPI_COMM_WORLD, root);
      }
      return true;
    }

    if (std::holds_alternative<std::vector<double>>(input)) {
      auto original_data = std::get<std::vector<double>>(input);

      if (rank == root) {
        auto &output_variant = GetOutput();
        auto &result_data = std::get<std::vector<double>>(output_variant);

        CustomReduce(original_data.data(), result_data.data(), static_cast<int>(original_data.size()), MPI_DOUBLE,
                     MPI_SUM, MPI_COMM_WORLD, root);

      } else {
        std::vector<double> temp_result(original_data.size());
        CustomReduce(original_data.data(), temp_result.data(), static_cast<int>(original_data.size()), MPI_DOUBLE,
                     MPI_SUM, MPI_COMM_WORLD, root);
      }
      return true;
    }

    return false;
  } catch (...) {
    return false;
  }
}

void KotelnikovaAFromAllToOneMPI::CustomReduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                                               MPI_Op op, MPI_Comm comm, int root) {
  int size = 0;
  MPI_Comm_size(comm, &size);

  int rank = 0;
  MPI_Comm_rank(comm, &rank);

  if (rank == root) {
    TreeReduce(sendbuf, recvbuf, count, datatype, op, comm, root);
  } else {
    int type_size = 0;
    MPI_Type_size(datatype, &type_size);
    size_t total_bytes = static_cast<size_t>(count) * static_cast<size_t>(type_size);
    std::vector<unsigned char> temp_buf(total_bytes);
    TreeReduce(sendbuf, temp_buf.data(), count, datatype, op, comm, root);
  }
}

void KotelnikovaAFromAllToOneMPI::TreeReduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                                             MPI_Comm comm, int root) {
  int size = 0;
  MPI_Comm_size(comm, &size);

  int rank = 0;
  MPI_Comm_rank(comm, &rank);

  if (count == 0 || root != 0 || op != MPI_SUM) {
    return;
  }

  int type_size = 0;
  MPI_Type_size(datatype, &type_size);
  size_t total_bytes = static_cast<size_t>(count) * static_cast<size_t>(type_size);

  std::vector<unsigned char> local_buf(total_bytes);
  std::memcpy(local_buf.data(), sendbuf, total_bytes);

  int depth = 0;
  while ((1 << depth) < size) {
    depth++;
  }

  for (int level = 0; level < depth; level++) {
    int mask = 1 << level;
    int partner = rank ^ mask;

    if (partner >= size) {
      continue;
    }

    if ((rank & mask) == 0) {
      if (partner < size) {
        std::vector<unsigned char> recv_buf(total_bytes);
        MPI_Recv(recv_buf.data(), count, datatype, partner, 0, comm, MPI_STATUS_IGNORE);
        PerformOperation(recv_buf.data(), local_buf.data(), count, datatype);
      }
    } else {
      MPI_Send(local_buf.data(), count, datatype, partner, 0, comm);
      break;
    }
  }

  if (rank == root && recvbuf != nullptr) {
    std::memcpy(recvbuf, local_buf.data(), total_bytes);
  }
}

void KotelnikovaAFromAllToOneMPI::PerformOperation(void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype) {
  if (datatype == MPI_INT) {
    auto *in = static_cast<int *>(inbuf);
    auto *inout = static_cast<int *>(inoutbuf);
    for (int i = 0; i < count; i++) {
      inout[i] += in[i];
    }
  } else if (datatype == MPI_FLOAT) {
    auto *in = static_cast<float *>(inbuf);
    auto *inout = static_cast<float *>(inoutbuf);
    for (int i = 0; i < count; i++) {
      inout[i] += in[i];
    }
  } else if (datatype == MPI_DOUBLE) {
    auto *in = static_cast<double *>(inbuf);
    auto *inout = static_cast<double *>(inoutbuf);
    for (int i = 0; i < count; i++) {
      inout[i] += in[i];
    }
  } else {
    throw std::runtime_error("Unsupported datatype");
  }
}

bool KotelnikovaAFromAllToOneMPI::PostProcessingImpl() {
  return true;
}

}  // namespace kotelnikova_a_from_all_to_one
