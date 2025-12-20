#include "kotelnikova_a_convex_hull_for_bin_image/mpi/include/ops_mpi.hpp"

#include <mpi.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <queue>
#include <utility>
#include <vector>

#include "kotelnikova_a_convex_hull_for_bin_image/common/include/common.hpp"

namespace kotelnikova_a_convex_hull_for_bin_image {

namespace {

int Cross(const Point &o, const Point &a, const Point &b) {
  return ((a.x - o.x) * (b.y - o.y)) - ((a.y - o.y) * (b.x - o.x));
}

}  // namespace

KotelnikovaAConvexHullForBinImgMPI::KotelnikovaAConvexHullForBinImgMPI(const InType &in) : local_data_(in) {
  SetTypeOfTask(GetStaticTypeOfTask());
  GetInput() = in;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
  MPI_Comm_size(MPI_COMM_WORLD, &size_);

  if (rank_ == 0) {
    full_data_ = in;
  }
}

bool KotelnikovaAConvexHullForBinImgMPI::ValidationImpl() {
  return GetInput().width > 0 && GetInput().height > 0 && !GetInput().pixels.empty() &&
         GetInput().pixels.size() == static_cast<size_t>(GetInput().width) * static_cast<size_t>(GetInput().height);
}

bool KotelnikovaAConvexHullForBinImgMPI::PreProcessingImpl() {
  const uint8_t threshold = 128;

  if (rank_ == 0) {
    int total_pixels = full_data_.width * full_data_.height;
    for (int i = 0; i < total_pixels; ++i) {
      if (full_data_.pixels[i] > threshold) {
        full_data_.pixels[i] = 255;
      } else {
        full_data_.pixels[i] = 0;
      }
    }
  }

  ScatterDataAndDistributeWork();

  return true;
}

void KotelnikovaAConvexHullForBinImgMPI::ScatterDataAndDistributeWork() {
  int width = 0, height = 0;

  if (rank_ == 0) {
    width = full_data_.width;
    height = full_data_.height;
  }

  MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);

  local_data_.width = width;
  local_data_.height = height;

  rows_per_proc_ = height / size_;
  int remainder = height % size_;

  start_row_ = (rank_ * rows_per_proc_) + std::min(rank_, remainder);
  end_row_ = start_row_ + rows_per_proc_ + (rank_ < remainder ? 1 : 0);
  int local_rows = end_row_ - start_row_;

  std::vector<int> send_counts(size_, 0);
  std::vector<int> displs(size_, 0);

  if (rank_ == 0) {
    for (int i = 0; i < size_; ++i) {
      int proc_start = (i * rows_per_proc_) + std::min(i, remainder);
      int proc_end = proc_start + rows_per_proc_ + (i < remainder ? 1 : 0);
      send_counts[i] = (proc_end - proc_start) * width;
      displs[i] = proc_start * width;
    }
  }

  size_t local_pixel_count = static_cast<size_t>(local_rows) * static_cast<size_t>(width);
  local_data_.pixels.resize(local_pixel_count);

  MPI_Scatterv(rank_ == 0 ? full_data_.pixels.data() : nullptr, send_counts.data(), displs.data(), MPI_UINT8_T,
               local_data_.pixels.data(), static_cast<int>(local_pixel_count), MPI_UINT8_T, 0, MPI_COMM_WORLD);
}

bool KotelnikovaAConvexHullForBinImgMPI::RunImpl() {
  FindConnectedComponentsMpi();

  ProcessComponentsAndComputeHulls();

  if (rank_ == 0) {
    local_data_.components = local_data_.convex_hulls;
    local_data_.convex_hulls.clear();
  }

  std::vector<int> hull_counts(size_, 0);
  int local_hull_count = static_cast<int>(local_data_.convex_hulls.size());
  MPI_Gather(&local_hull_count, 1, MPI_INT, hull_counts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

  if (rank_ == 0) {
    for (int i = 1; i < size_; ++i) {
      for (int j = 0; j < hull_counts[i]; ++j) {
        int hull_size = 0;
        MPI_Recv(&hull_size, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (hull_size > 0) {
          std::vector<Point> hull(hull_size);
          std::vector<int> point_data(static_cast<size_t>(hull_size) * 2);
          MPI_Recv(point_data.data(), hull_size * 2, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

          for (int k = 0; k < hull_size; ++k) {
            hull[k].x = point_data[static_cast<size_t>(k) * 2];
            hull[k].y = point_data[static_cast<size_t>(k) * 2 + 1];
          }

          local_data_.convex_hulls.push_back(hull);
        }
      }
    }

    for (const auto &hull : local_data_.components) {
      local_data_.convex_hulls.push_back(hull);
    }
  } else {
    for (const auto &hull : local_data_.convex_hulls) {
      int hull_size = static_cast<int>(hull.size());
      MPI_Send(&hull_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

      if (hull_size > 0) {
        std::vector<int> point_data;
        point_data.reserve(static_cast<size_t>(hull_size) * 2);
        for (const auto &point : hull) {
          point_data.push_back(point.x);
          point_data.push_back(point.y);
        }

        MPI_Send(point_data.data(), hull_size * 2, MPI_INT, 0, 0, MPI_COMM_WORLD);
      }
    }
  }

  GetOutput() = local_data_;

  return true;
}

void KotelnikovaAConvexHullForBinImgMPI::ProcessComponentsAndComputeHulls() {
  local_data_.convex_hulls.clear();

  for (const auto &component : local_data_.components) {
    if (component.size() >= 3) {
      local_data_.convex_hulls.push_back(GrahamScan(component));
    } else if (!component.empty()) {
      local_data_.convex_hulls.push_back(component);
    }
  }
}

void KotelnikovaAConvexHullForBinImgMPI::FindConnectedComponentsMpi() {
  int width = local_data_.width;
  int local_rows = end_row_ - start_row_;

  std::vector<bool> visited_local(static_cast<size_t>(width) * local_rows, false);
  std::vector<std::vector<Point>> local_components;

  for (int row_y = 0; row_y < local_rows; ++row_y) {
    int global_row_y = start_row_ + row_y;
    for (int col_x = 0; col_x < width; ++col_x) {
      int local_idx = (row_y * width) + col_x;
      int global_idx = (global_row_y * width) + col_x;

      if (local_data_.pixels[global_idx] == 255 && !visited_local[local_idx]) {
        std::vector<Point> component;
        std::queue<Point> q;
        q.emplace(col_x, global_row_y);
        visited_local[local_idx] = true;

        while (!q.empty()) {
          Point p = q.front();
          q.pop();
          component.push_back(p);

          const std::vector<std::pair<int, int>> directions = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

          for (const auto &dir : directions) {
            int nx = p.x + dir.first;
            int ny = p.y + dir.second;

            if (nx >= 0 && nx < width && ny >= start_row_ && ny < end_row_) {
              int nlocal_idx = ((ny - start_row_) * width) + nx;
              int nglobal_idx = (ny * width) + nx;

              if (local_data_.pixels[nglobal_idx] == 255 && !visited_local[nlocal_idx]) {
                visited_local[nlocal_idx] = true;
                q.emplace(nx, ny);
              }
            }
          }
        }

        if (!component.empty()) {
          local_components.push_back(component);
        }
      }
    }
  }

  local_data_.components = local_components;
}

bool KotelnikovaAConvexHullForBinImgMPI::PostProcessingImpl() {
  return true;
}

std::vector<Point> KotelnikovaAConvexHullForBinImgMPI::GrahamScan(const std::vector<Point> &points) {
  if (points.size() <= 3) {
    return points;
  }

  std::vector<Point> pts = points;
  int n = static_cast<int>(pts.size());

  int min_idx = 0;
  for (int i = 1; i < n; ++i) {
    if (pts[i].y < pts[min_idx].y || (pts[i].y == pts[min_idx].y && pts[i].x < pts[min_idx].x)) {
      min_idx = i;
    }
  }
  std::swap(pts[0], pts[min_idx]);

  Point pivot = pts[0];
  std::sort(pts.begin() + 1, pts.end(), [&pivot](const Point &a, const Point &b) {
    int orient = Cross(pivot, a, b);
    if (orient == 0) {
      return ((a.x - pivot.x) * (a.x - pivot.x)) + ((a.y - pivot.y) * (a.y - pivot.y)) <
             ((b.x - pivot.x) * (b.x - pivot.x)) + ((b.y - pivot.y) * (b.y - pivot.y));
    }
    return orient > 0;
  });

  std::vector<Point> hull;
  hull.reserve(pts.size());
  for (int i = 0; i < n; ++i) {
    while (hull.size() >= 2 && Cross(hull[hull.size() - 2], hull.back(), pts[i]) <= 0) {
      hull.pop_back();
    }
    hull.push_back(pts[i]);
  }

  return hull;
}

}  // namespace kotelnikova_a_convex_hull_for_bin_image
