#pragma once

#include <vector>

#include "kotelnikova_a_convex_hull_for_bin_image/common/include/common.hpp"
#include "task/include/task.hpp"

namespace kotelnikova_a_convex_hull_for_bin_image {

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

  void FindConnectedComponentsMpi();
  void ProcessComponentsAndComputeHulls();
  static std::vector<Point> GrahamScan(const std::vector<Point> &points);

  void ScatterDataAndDistributeWork();

  ImageData local_data_;
  ImageData full_data_;
  int rank_{0}, size_{0};

  int start_row_{0}, end_row_{0};
  int rows_per_proc_{0};
};

}  // namespace kotelnikova_a_convex_hull_for_bin_image
