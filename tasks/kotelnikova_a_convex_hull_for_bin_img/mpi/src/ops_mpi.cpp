#include "kotelnikova_a_convex_hull_for_bin_img/mpi/include/ops_mpi.hpp"

#include <mpi.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <queue>
#include <ranges>
#include <utility>
#include <vector>

#include "kotelnikova_a_convex_hull_for_bin_img/common/include/common.hpp"

namespace kotelnikova_a_convex_hull_for_bin_img {

namespace {

int Cross(const Point &o, const Point &a, const Point &b) {
  return ((a.x - o.x) * (b.y - o.y)) - ((a.y - o.y) * (b.x - o.x));
}

void ProcessNeighbors(const Point &p, int width, int start_row, int end_row, const ImageData &local_data,
                      std::vector<bool> &visited_local, std::queue<Point> &q) {
  const std::vector<std::pair<int, int>> directions = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

  for (const auto &dir : directions) {
    int nx = p.x + dir.first;
    int ny = p.y + dir.second;

    if (nx >= 0 && nx < width && ny >= start_row && ny < end_row) {
      int nlocal_idx = ((ny - start_row) * width) + nx;
      int nglobal_idx = (ny * width) + nx;

      if (local_data.pixels[nglobal_idx] == 255 && !visited_local[nlocal_idx]) {
        visited_local[nlocal_idx] = true;
        q.emplace(nx, ny);
      }
    }
  }
}

void ProcessLocalRegion(int start_row, int end_row, int width, const ImageData &local_data,
                        std::vector<std::vector<Point>> &local_components) {
  std::vector<bool> visited_local(static_cast<size_t>(width) * (end_row - start_row), false);

  for (int row_y = start_row; row_y < end_row; ++row_y) {
    for (int col_x = 0; col_x < width; ++col_x) {
      int local_idx = ((row_y - start_row) * width) + col_x;
      int global_idx = (row_y * width) + col_x;

      if (local_data.pixels[global_idx] == 255 && !visited_local[local_idx]) {
        std::vector<Point> component;
        std::queue<Point> q;
        q.emplace(col_x, row_y);
        visited_local[local_idx] = true;

        while (!q.empty()) {
          Point p = q.front();
          q.pop();
          component.push_back(p);

          ProcessNeighbors(p, width, start_row, end_row, local_data, visited_local, q);
        }

        if (!component.empty()) {
          local_components.push_back(component);
        }
      }
    }
  }
}

void GatherComponentsFromRank0(std::vector<std::vector<Point>> &components) {
  int size = 0;
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  for (int i = 1; i < size; ++i) {
    int comp_count = 0;
    MPI_Recv(&comp_count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    for (int j = 0; j < comp_count; ++j) {
      int comp_size = 0;
      MPI_Recv(&comp_size, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      std::vector<Point> component(comp_size);
      MPI_Recv(component.data(), comp_size * 2, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      components.push_back(component);
    }
  }
}

void SendComponentsToRank0(const std::vector<std::vector<Point>> &local_components) {
  int comp_count = static_cast<int>(local_components.size());
  MPI_Send(&comp_count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

  for (const auto &component : local_components) {
    int comp_size = static_cast<int>(component.size());
    MPI_Send(&comp_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

    std::vector<int> point_data;
    point_data.reserve(static_cast<size_t>(comp_size) * 2);
    for (const auto &point : component) {
      point_data.push_back(point.x);
      point_data.push_back(point.y);
    }

    MPI_Send(point_data.data(), comp_size * 2, MPI_INT, 0, 0, MPI_COMM_WORLD);
  }
}

std::vector<char> SerializeConvexHulls(const std::vector<std::vector<Point>> &convex_hulls) {
  int hull_count = static_cast<int>(convex_hulls.size());

  size_t total_size = sizeof(int);

  for (const auto &hull : convex_hulls) {
    total_size += sizeof(int);
    total_size += hull.size() * sizeof(Point);
  }

  std::vector<char> buffer(total_size);

  if (total_size == 0) {
    return buffer;
  }

  char *ptr = buffer.data();

  *reinterpret_cast<int *>(ptr) = hull_count;
  ptr += sizeof(hull_count);

  for (const auto &hull : convex_hulls) {
    int hull_size = static_cast<int>(hull.size());
    *reinterpret_cast<int *>(ptr) = hull_size;
    ptr += sizeof(hull_size);

    if (hull_size > 0) {
      auto point_ptr = reinterpret_cast<Point *>(ptr);
      std::ranges::copy(hull, point_ptr);
      ptr += hull_size * sizeof(Point);
    }
  }

  return buffer;
}

std::vector<std::vector<Point>> DeserializeConvexHulls(const char *buffer) {
  if (buffer == nullptr) {
    return {};
  }

  std::vector<std::vector<Point>> convex_hulls;

  int hull_count;
  std::memcpy(&hull_count, buffer, sizeof(hull_count));
  const char *ptr = buffer + sizeof(hull_count);

  convex_hulls.reserve(hull_count);

  for (int i = 0; i < hull_count; ++i) {
    int hull_size;
    std::memcpy(&hull_size, ptr, sizeof(hull_size));
    ptr += sizeof(hull_size);

    std::vector<Point> hull(hull_size);
    if (hull_size > 0) {
      auto point_ptr = reinterpret_cast<const Point *>(ptr);
      std::ranges::copy(point_ptr, point_ptr + hull_size, hull.begin());
      ptr += hull_size * sizeof(Point);
    }

    convex_hulls.push_back(hull);
  }

  return convex_hulls;
}

}  // namespace

KotelnikovaAConvexHullForBinImgMPI::KotelnikovaAConvexHullForBinImgMPI(const InType &in) : local_data_(in) {
  SetTypeOfTask(GetStaticTypeOfTask());
  GetInput() = in;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
  MPI_Comm_size(MPI_COMM_WORLD, &size_);
}

bool KotelnikovaAConvexHullForBinImgMPI::ValidationImpl() {
  return GetInput().width > 0 && GetInput().height > 0 && !GetInput().pixels.empty() &&
         GetInput().pixels.size() == static_cast<size_t>(GetInput().width) * static_cast<size_t>(GetInput().height);
}

bool KotelnikovaAConvexHullForBinImgMPI::PreProcessingImpl() {
  BinarizeImageMpi();
  return true;
}

bool KotelnikovaAConvexHullForBinImgMPI::RunImpl() {
  FindConnectedComponentsMpi();

  if (rank_ == 0) {
    ProcessConvexHulls();
  }

  BroadcastConvexHulls();
  BroadcastComponents();

  GetOutput() = local_data_;
  return true;
}

bool KotelnikovaAConvexHullForBinImgMPI::PostProcessingImpl() {
  return true;
}

void KotelnikovaAConvexHullForBinImgMPI::BinarizeImageMpi() {
  const uint8_t threshold = 128;
  int total_pixels = local_data_.width * local_data_.height;

  int pixels_per_proc = total_pixels / size_;
  int remainder = total_pixels % size_;

  int start_idx = (rank_ * pixels_per_proc) + std::min(rank_, remainder);
  int end_idx = start_idx + pixels_per_proc + (rank_ < remainder ? 1 : 0);

  for (int i = start_idx; i < end_idx; ++i) {
    if (local_data_.pixels[i] > threshold) {
      local_data_.pixels[i] = 255;
    } else {
      local_data_.pixels[i] = 0;
    }
  }

  std::vector<int> recv_counts(size_);
  std::vector<int> displs(size_);

  for (int i = 0; i < size_; ++i) {
    int proc_start = (i * pixels_per_proc) + std::min(i, remainder);
    int proc_end = proc_start + pixels_per_proc + (i < remainder ? 1 : 0);
    recv_counts[i] = proc_end - proc_start;
    displs[i] = proc_start;
  }

  MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, local_data_.pixels.data(), recv_counts.data(), displs.data(),
                 MPI_UINT8_T, MPI_COMM_WORLD);
}

void KotelnikovaAConvexHullForBinImgMPI::FindConnectedComponentsMpi() {
  int width = local_data_.width;
  int height = local_data_.height;

  int rows_per_proc = height / size_;
  int remainder = height % size_;

  int start_row = (rank_ * rows_per_proc) + std::min(rank_, remainder);
  int end_row = start_row + rows_per_proc + (rank_ < remainder ? 1 : 0);

  std::vector<std::vector<Point>> local_components;
  ProcessLocalRegion(start_row, end_row, width, local_data_, local_components);

  if (rank_ == 0) {
    local_data_.components = local_components;
    GatherComponentsFromRank0(local_data_.components);
  } else {
    SendComponentsToRank0(local_components);
  }
}

void KotelnikovaAConvexHullForBinImgMPI::ProcessConvexHulls() {
  local_data_.convex_hulls.clear();
  for (const auto &component : local_data_.components) {
    if (component.size() >= 3) {
      local_data_.convex_hulls.push_back(GrahamScan(component));
    } else if (!component.empty()) {
      local_data_.convex_hulls.push_back(component);
    }
  }
}

void KotelnikovaAConvexHullForBinImgMPI::BroadcastConvexHulls() {
  if (rank_ == 0) {
    std::vector<char> serialized_data = SerializeConvexHulls(local_data_.convex_hulls);
    int data_size = static_cast<int>(serialized_data.size());

    MPI_Bcast(&data_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (data_size > 0) {
      MPI_Bcast(serialized_data.data(), data_size, MPI_BYTE, 0, MPI_COMM_WORLD);
    }
  } else {
    int data_size = 0;
    MPI_Bcast(&data_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (data_size > 0) {
      std::vector<char> received_data(data_size);
      MPI_Bcast(received_data.data(), data_size, MPI_BYTE, 0, MPI_COMM_WORLD);
      local_data_.convex_hulls = DeserializeConvexHulls(received_data.data());
    } else {
      local_data_.convex_hulls.clear();
    }
  }
}

void KotelnikovaAConvexHullForBinImgMPI::BroadcastComponents() {
  if (rank_ == 0) {
    std::vector<char> serialized_comps = SerializeConvexHulls(local_data_.components);
    int comp_data_size = static_cast<int>(serialized_comps.size());

    MPI_Bcast(&comp_data_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (comp_data_size > 0) {
      MPI_Bcast(serialized_comps.data(), comp_data_size, MPI_BYTE, 0, MPI_COMM_WORLD);
    }
  } else {
    int comp_data_size = 0;
    MPI_Bcast(&comp_data_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (comp_data_size > 0) {
      std::vector<char> received_comps(comp_data_size);
      MPI_Bcast(received_comps.data(), comp_data_size, MPI_BYTE, 0, MPI_COMM_WORLD);
      local_data_.components = DeserializeConvexHulls(received_comps.data());
    } else {
      local_data_.components.clear();
    }
  }
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
  for (int i = 0; i < n; ++i) {
    while (hull.size() >= 2 && Cross(hull[hull.size() - 2], hull.back(), pts[i]) <= 0) {
      hull.pop_back();
    }
    hull.push_back(pts[i]);
  }

  return hull;
}
}  // namespace kotelnikova_a_convex_hull_for_bin_img
