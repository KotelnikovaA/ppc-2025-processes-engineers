#include "kotelnikova_a_convex_hull_for_bin_img/mpi/include/ops_mpi.hpp"

#include <mpi.h>

#include <algorithm>
#include <queue>
#include <set>

namespace kotelnikova_a_convex_hull_for_bin_img {

static int cross(const Point &O, const Point &A, const Point &B) {
  return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

KotelnikovaAConvexHullForBinImgMPI::KotelnikovaAConvexHullForBinImgMPI(const InType &in) {
  SetTypeOfTask(GetStaticTypeOfTask());
  GetInput() = in;
  local_data_ = in;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
  MPI_Comm_size(MPI_COMM_WORLD, &size_);
}

bool KotelnikovaAConvexHullForBinImgMPI::ValidationImpl() {
  return GetInput().width > 0 && GetInput().height > 0 && !GetInput().pixels.empty() &&
         GetInput().pixels.size() == static_cast<size_t>(GetInput().width * GetInput().height);
}

bool KotelnikovaAConvexHullForBinImgMPI::PreProcessingImpl() {
  binarizeImageMPI();
  return true;
}

bool KotelnikovaAConvexHullForBinImgMPI::RunImpl() {
  findConnectedComponentsMPI();

  if (rank_ == 0) {
    local_data_.convex_hulls.clear();
    for (const auto &component : local_data_.components) {
      if (component.size() >= 3) {
        local_data_.convex_hulls.push_back(grahamScan(component));
      } else if (component.size() > 0) {
        local_data_.convex_hulls.push_back(component);
      }
    }
  }

  GetOutput() = local_data_;
  return true;
}

bool KotelnikovaAConvexHullForBinImgMPI::PostProcessingImpl() {
  return true;
}

void KotelnikovaAConvexHullForBinImgMPI::binarizeImageMPI() {
  const uint8_t threshold = 128;
  int total_pixels = local_data_.width * local_data_.height;

  int pixels_per_proc = total_pixels / size_;
  int remainder = total_pixels % size_;

  int start_idx = rank_ * pixels_per_proc + std::min(rank_, remainder);
  int end_idx = start_idx + pixels_per_proc + (rank_ < remainder ? 1 : 0);

  int local_count = 0;
  for (int i = start_idx; i < end_idx; ++i) {
    if (local_data_.pixels[i] > threshold) {
      local_data_.pixels[i] = 255;
      local_count++;
    } else {
      local_data_.pixels[i] = 0;
    }
  }

  std::vector<int> recv_counts(size_);
  std::vector<int> displs(size_);

  for (int i = 0; i < size_; ++i) {
    int proc_start = i * pixels_per_proc + std::min(i, remainder);
    int proc_end = proc_start + pixels_per_proc + (i < remainder ? 1 : 0);
    recv_counts[i] = proc_end - proc_start;
    displs[i] = proc_start;
  }

  MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, local_data_.pixels.data(), recv_counts.data(), displs.data(),
                 MPI_UINT8_T, MPI_COMM_WORLD);
}

void KotelnikovaAConvexHullForBinImgMPI::findConnectedComponentsMPI() {
  int width = local_data_.width;
  int height = local_data_.height;

  int rows_per_proc = height / size_;
  int remainder = height % size_;

  int start_row = rank_ * rows_per_proc + std::min(rank_, remainder);
  int end_row = start_row + rows_per_proc + (rank_ < remainder ? 1 : 0);

  std::vector<bool> visited_local(width * (end_row - start_row), false);
  std::vector<std::vector<Point>> local_components;

  std::vector<std::pair<int, int>> directions = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

  int found_components = 0;
  for (int y = start_row; y < end_row; ++y) {
    for (int x = 0; x < width; ++x) {
      int local_idx = (y - start_row) * width + x;
      int global_idx = y * width + x;

      if (local_data_.pixels[global_idx] == 255 && !visited_local[local_idx]) {
        std::vector<Point> component;
        std::queue<Point> q;
        q.push(Point(x, y));
        visited_local[local_idx] = true;

        while (!q.empty()) {
          Point p = q.front();
          q.pop();
          component.push_back(p);

          for (const auto &dir : directions) {
            int nx = p.x + dir.first;
            int ny = p.y + dir.second;

            if (nx >= 0 && nx < width && ny >= start_row && ny < end_row) {
              int nlocal_idx = (ny - start_row) * width + nx;
              int nglobal_idx = ny * width + nx;

              if (local_data_.pixels[nglobal_idx] == 255 && !visited_local[nlocal_idx]) {
                visited_local[nlocal_idx] = true;
                q.push(Point(nx, ny));
              }
            }
          }
        }

        if (!component.empty()) {
          local_components.push_back(component);
          found_components++;
        }
      }
    }
  }

  if (rank_ == 0) {
    local_data_.components = local_components;

    for (int i = 1; i < size_; ++i) {
      int comp_count;
      MPI_Recv(&comp_count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      for (int j = 0; j < comp_count; ++j) {
        int comp_size;
        MPI_Recv(&comp_size, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        std::vector<Point> component(comp_size);
        MPI_Recv(component.data(), comp_size * 2, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        local_data_.components.push_back(component);
      }
    }
  } else {
    int comp_count = static_cast<int>(local_components.size());
    MPI_Send(&comp_count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

    for (const auto &component : local_components) {
      int comp_size = static_cast<int>(component.size());
      MPI_Send(&comp_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

      std::vector<int> point_data;
      point_data.reserve(comp_size * 2);
      for (const auto &point : component) {
        point_data.push_back(point.x);
        point_data.push_back(point.y);
      }

      MPI_Send(point_data.data(), comp_size * 2, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
  }
}

std::vector<Point> KotelnikovaAConvexHullForBinImgMPI::grahamScan(const std::vector<Point> &points) {
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
    int orient = cross(pivot, a, b);
    if (orient == 0) {
      return (a.x - pivot.x) * (a.x - pivot.x) + (a.y - pivot.y) * (a.y - pivot.y) <
             (b.x - pivot.x) * (b.x - pivot.x) + (b.y - pivot.y) * (b.y - pivot.y);
    }
    return orient > 0;
  });

  std::vector<Point> hull;
  for (int i = 0; i < n; ++i) {
    while (hull.size() >= 2 && cross(hull[hull.size() - 2], hull.back(), pts[i]) <= 0) {
      hull.pop_back();
    }
    hull.push_back(pts[i]);
  }

  return hull;
}
}  // namespace kotelnikova_a_convex_hull_for_bin_img
