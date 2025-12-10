#pragma once

#include <mpi.h>

#include "kotelnikova_a_from_all_to_one/common/include/common.hpp"
#include "task/include/task.hpp"

namespace kotelnikova_a_from_all_to_one {

class KotelnikovaAFromAllToOneMPI : public BaseTask {
 public:
  static constexpr ppc::task::TypeOfTask GetStaticTypeOfTask() {
    return ppc::task::TypeOfTask::kMPI;
  }
  explicit KotelnikovaAFromAllToOneMPI(const InType &in);

  static void CustomReduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                           int root);

 private:
  template <typename T>
  bool ProcessVector(const InType &input, int rank, int root, MPI_Datatype mpi_type);

  bool ValidationImpl() override;
  bool PreProcessingImpl() override;
  bool RunImpl() override;
  bool PostProcessingImpl() override;

  static void TreeReduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                         int root);
  static void PerformOperation(void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype);
};

}  // namespace kotelnikova_a_from_all_to_one
