#include "kotelnikova_a_from_all_to_one/mpi/include/ops_mpi.hpp"

#include <mpi.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <variant>
#include <vector>

#include "kotelnikova_a_from_all_to_one/common/include/common.hpp"

namespace {
void PerformOperationImpl(void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype) {
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
}  // namespace

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
    } else {
      throw std::runtime_error("Unsupported data type");
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
    int root = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (std::holds_alternative<std::vector<int>>(input)) {
      return ProcessVector<int>(input, rank, root, MPI_INT);
    }

    if (std::holds_alternative<std::vector<float>>(input)) {
      return ProcessVector<float>(input, rank, root, MPI_FLOAT);
    }

    if (std::holds_alternative<std::vector<double>>(input)) {
      return ProcessVector<double>(input, rank, root, MPI_DOUBLE);
    }

    return false;
  } catch (...) {
    return false;
  }
}

template <typename T>
bool KotelnikovaAFromAllToOneMPI::ProcessVector(const InType &input, int rank, int root, MPI_Datatype mpi_type) {
  auto &original_data = std::get<std::vector<T>>(input);

  if (original_data.empty()) {
    return true;
  }

  if (rank == root) {
    auto &output_variant = GetOutput();
    auto &result_data = std::get<std::vector<T>>(output_variant);
    std::ranges::copy(original_data, result_data.begin());
    CustomReduce(result_data.data(), result_data.data(), static_cast<int>(original_data.size()), mpi_type, MPI_SUM,
                 MPI_COMM_WORLD, root);
  } else {
    std::vector<T> send_buffer = original_data;
    CustomReduce(send_buffer.data(), nullptr, static_cast<int>(original_data.size()), mpi_type, MPI_SUM, MPI_COMM_WORLD,
                 root);
  }
  return true;
}

void KotelnikovaAFromAllToOneMPI::CustomReduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                                               MPI_Op op, MPI_Comm comm, int root) {
  if (count == 0) {
    MPI_Barrier(comm);
    return;
  }

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

  if (count == 0) {
    MPI_Barrier(comm);
    return;
  }

  if (op != MPI_SUM) {
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
  PerformOperationImpl(inbuf, inoutbuf, count, datatype);
}

bool KotelnikovaAFromAllToOneMPI::PostProcessingImpl() {
  return true;
}

template bool KotelnikovaAFromAllToOneMPI::ProcessVector<int>(const InType &input, int rank, int root,
                                                              MPI_Datatype mpi_type);
template bool KotelnikovaAFromAllToOneMPI::ProcessVector<float>(const InType &input, int rank, int root,
                                                                MPI_Datatype mpi_type);
template bool KotelnikovaAFromAllToOneMPI::ProcessVector<double>(const InType &input, int rank, int root,
                                                                 MPI_Datatype mpi_type);

}  // namespace kotelnikova_a_from_all_to_one
