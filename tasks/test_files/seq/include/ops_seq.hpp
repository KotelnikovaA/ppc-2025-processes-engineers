#pragma once

#include <vector>

#include "task/include/task.hpp"
#include "test_files/common/include/common.hpp"

namespace test_files {

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
  static std::vector<Point> GrahamScan(const std::vector<Point> &points);
  void BinarizeImage();

  ImageData processed_data_;
};

}  // namespace test_files
