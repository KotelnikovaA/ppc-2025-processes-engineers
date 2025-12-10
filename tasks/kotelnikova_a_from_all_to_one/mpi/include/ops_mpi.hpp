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

  void TreeReduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, int root);
  void CustomReduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, int root);
  void PerformOperation(void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype);

 private:
  template <typename T, MPI_Datatype MpiType>
  bool ProcessVector(const InType &input, int rank, int root);

  bool ValidationImpl() override;
  bool PreProcessingImpl() override;
  bool RunImpl() override;
  bool PostProcessingImpl() override;
};

}  // namespace kotelnikova_a_from_all_to_one
