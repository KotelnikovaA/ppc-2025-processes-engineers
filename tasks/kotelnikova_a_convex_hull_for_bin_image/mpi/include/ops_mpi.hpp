#pragma once

#include <queue>
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
  void GatherConvexHullsToRank0();
  void ReceiveHullsFromProcess(int source_rank, int hull_count);
  void SendHullsToRank0();
  static std::vector<Point> GrahamScan(const std::vector<Point> &points);

  void ScatterDataAndDistributeWork();
  void ProcessImageRegion(int width, int local_rows, std::vector<bool> &visited_local,
                          std::vector<std::vector<Point>> &local_components);
  void ProcessPixel(int col_x, int global_row_y, int local_row_y, int width, std::vector<bool> &visited_local,
                    std::vector<std::vector<Point>> &local_components);
  void ProcessPixelNeighbors(const Point &p, int width, std::vector<bool> &visited_local, std::queue<Point> &q);

  ImageData local_data_;
  ImageData full_data_;
  int rank_{0}, size_{0};

  int start_row_{0}, end_row_{0};
  int rows_per_proc_{0};
};

}  // namespace kotelnikova_a_convex_hull_for_bin_image
