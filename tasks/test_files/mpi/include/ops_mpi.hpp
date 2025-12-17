#pragma once

#include <vector>

#include "task/include/task.hpp"
#include "test_files/common/include/common.hpp"

namespace test_files {

class KotelnikovaAConvexHullForBinImgMPI : public BaseTask {
 public:
  static constexpr ppc::task::TypeOfTask GetStaticTypeOfTask() {
    return ppc::task::TypeOfTask::kMPI;
  }
  explicit KotelnikovaAConvexHullForBinImgMPI(const InType &in);

 private:
  bool ValidationImpl() override;
  bool PreProcessingImpl() override;
  bool RunImpl() override;
  bool PostProcessingImpl() override;

  void BinarizeImageMpi();
  void FindConnectedComponentsMpi();
  static std::vector<Point> GrahamScan(const std::vector<Point> &points);

  ImageData local_data_;
  int rank_{0}, size_{0};
};

}  // namespace test_files
