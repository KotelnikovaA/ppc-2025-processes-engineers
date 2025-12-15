#pragma once

#include <vector>

#include "kotelnikova_a_convex_hull_for_bin_img/common/include/common.hpp"
#include "task/include/task.hpp"

namespace kotelnikova_a_convex_hull_for_bin_img {

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
  void ProcessConvexHulls();
  void BroadcastConvexHulls();
  void BroadcastComponents();
  static std::vector<Point> GrahamScan(const std::vector<Point> &points);

  ImageData local_data_;
  int rank_{0}, size_{0};
};

}  // namespace kotelnikova_a_convex_hull_for_bin_img
