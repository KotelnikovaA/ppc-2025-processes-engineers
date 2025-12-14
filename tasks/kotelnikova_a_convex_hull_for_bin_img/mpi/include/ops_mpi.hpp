#pragma once

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

  void binarizeImageMPI();
  void findConnectedComponentsMPI();
  std::vector<Point> grahamScan(const std::vector<Point> &points);

  ImageData local_data_;
  int rank_, size_;
};

}  // namespace kotelnikova_a_convex_hull_for_bin_img
