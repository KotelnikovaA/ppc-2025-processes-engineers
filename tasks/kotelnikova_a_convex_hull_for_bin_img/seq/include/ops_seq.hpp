#pragma once

#include <vector>

#include "kotelnikova_a_convex_hull_for_bin_img/common/include/common.hpp"
#include "task/include/task.hpp"

namespace kotelnikova_a_convex_hull_for_bin_img {

class KotelnikovaAConvexHullForBinImgSEQ : public BaseTask {
 public:
  static constexpr ppc::task::TypeOfTask GetStaticTypeOfTask() {
    return ppc::task::TypeOfTask::kSEQ;
  }
  explicit KotelnikovaAConvexHullForBinImgSEQ(const InType &in);

 private:
  bool ValidationImpl() override;
  bool PreProcessingImpl() override;
  bool RunImpl() override;
  bool PostProcessingImpl() override;

  void FindConnectedComponents();
  std::vector<Point> GrahamScan(const std::vector<Point> &points);
  void BinarizeImage();

  ImageData processed_data_;
};

}  // namespace kotelnikova_a_convex_hull_for_bin_img
