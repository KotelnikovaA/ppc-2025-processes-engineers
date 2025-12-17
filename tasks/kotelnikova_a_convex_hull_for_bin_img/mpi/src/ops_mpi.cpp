#include "kotelnikova_a_convex_hull_for_bin_img/mpi/include/ops_mpi.hpp"

#include <mpi.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <queue>
#include <set>
#include <utility>
#include <vector>

namespace kotelnikova_a_convex_hull_for_bin_img {

namespace {

int Cross(const Point &o, const Point &a, const Point &b) {
  return ((a.x - o.x) * (b.y - o.y)) - ((a.y - o.y) * (b.x - o.x));
}

std::vector<Point> ProcessComponent(int start_x, int start_y, int width, int start_row, int end_row,
                                    const ImageData &local_data, std::vector<bool> &visited_local) {
  std::vector<Point> component;
  std::queue<Point> q;
  q.emplace(start_x, start_y);

  int local_idx = ((start_y - start_row) * width) + start_x;
  visited_local[static_cast<size_t>(local_idx)] = true;

  while (!q.empty()) {
    Point p = q.front();
    q.pop();
    component.push_back(p);

    const std::vector<std::pair<int, int>> directions = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (const auto &dir : directions) {
      int nx = p.x + dir.first;
      int ny = p.y + dir.second;

      if (nx >= 0 && nx < width && ny >= start_row && ny < end_row) {
        int nlocal_idx = ((ny - start_row) * width) + nx;
        int nglobal_idx = (ny * width) + nx;

        if (local_data.pixels[static_cast<size_t>(nglobal_idx)] == 255 &&
            !visited_local[static_cast<size_t>(nlocal_idx)]) {
          visited_local[static_cast<size_t>(nlocal_idx)] = true;
          q.emplace(nx, ny);
        }
      }
    }
  }

  return component;
}

void ProcessLocalRegion(int start_row, int end_row, int width, const ImageData &local_data,
                        std::vector<std::vector<Point>> &local_components, std::vector<Point> &border_pixels) {
  std::vector<bool> visited_local(static_cast<size_t>(width) * static_cast<size_t>(end_row - start_row), false);

  for (int row_y = start_row; row_y < end_row; ++row_y) {
    for (int col_x = 0; col_x < width; ++col_x) {
      int local_idx = ((row_y - start_row) * width) + col_x;
      int global_idx = (row_y * width) + col_x;

      if (local_data.pixels[static_cast<size_t>(global_idx)] == 255 && !visited_local[static_cast<size_t>(local_idx)]) {
        std::vector<Point> component =
            ProcessComponent(col_x, row_y, width, start_row, end_row, local_data, visited_local);

        if (!component.empty()) {
          local_components.push_back(component);

          bool touches_border = false;
          for (const auto &point : component) {
            if (point.y == start_row || point.y == end_row - 1) {
              touches_border = true;
              break;
            }
          }

          if (touches_border) {
            for (const auto &point : component) {
              if (point.y == start_row || point.y == end_row - 1) {
                border_pixels.push_back(point);
              }
            }
          }
        }
      }
    }
  }
}

void ExchangeBorderPixels(int size, std::vector<Point> &border_pixels,
                          std::vector<std::vector<Point>> &received_borders) {
  int local_count = static_cast<int>(border_pixels.size()) * 2;

  std::vector<int> all_counts(static_cast<size_t>(size));
  MPI_Allgather(&local_count, 1, MPI_INT, all_counts.data(), 1, MPI_INT, MPI_COMM_WORLD);

  std::vector<int> displs(static_cast<size_t>(size), 0);
  int total_size = 0;
  for (int i = 0; i < size; ++i) {
    displs[static_cast<size_t>(i)] = total_size;
    total_size += all_counts[static_cast<size_t>(i)];
  }

  std::vector<int> local_data;
  local_data.reserve(static_cast<size_t>(local_count));
  for (const auto &p : border_pixels) {
    local_data.push_back(p.x);
    local_data.push_back(p.y);
  }

  std::vector<int> all_data(static_cast<size_t>(total_size));
  MPI_Allgatherv(local_data.data(), local_count, MPI_INT, all_data.data(), all_counts.data(), displs.data(), MPI_INT,
                 MPI_COMM_WORLD);

  received_borders.resize(static_cast<size_t>(size));
  for (int i = 0; i < size; ++i) {
    int count = all_counts[static_cast<size_t>(i)] / 2;
    received_borders[static_cast<size_t>(i)].reserve(static_cast<size_t>(count));
    int start = displs[static_cast<size_t>(i)];
    for (int j = 0; j < count; ++j) {
      int x = all_data[static_cast<size_t>(start) + static_cast<size_t>(j) * 2];
      int y = all_data[static_cast<size_t>(start) + (static_cast<size_t>(j) * 2) + 1];
      received_borders[static_cast<size_t>(i)].emplace_back(x, y);
    }
  }
}

bool HasBorderPoint(const std::vector<Point> &component, const std::set<std::pair<int, int>> &border_set) {
  for (const auto &point : component) {
    if (border_set.contains({point.x, point.y})) {
      return true;
    }
  }
  return false;
}

void MergeBorderComponents(std::vector<std::vector<Point>> &local_components,
                           const std::vector<std::vector<Point>> &received_borders, int current_rank, int start_row,
                           int end_row) {
  if (local_components.empty()) {
    return;
  }

  std::set<std::pair<int, int>> local_border_set;

  for (const auto &component : local_components) {
    for (const auto &point : component) {
      if (point.y == start_row || point.y == end_row - 1) {
        local_border_set.insert({point.x, point.y});
      }
    }
  }

  std::vector<std::pair<int, int>> all_border_points;
  for (size_t proc = 0; proc < received_borders.size(); ++proc) {
    if (static_cast<int>(proc) == current_rank) {
      continue;
    }
    for (const auto &border_point : received_borders[proc]) {
      if (local_border_set.contains({border_point.x, border_point.y})) {
        all_border_points.emplace_back(border_point.x, border_point.y);
      }
    }
  }

  if (all_border_points.empty()) {
    return;
  }

  bool changed = false;
  std::vector<bool> merged(local_components.size(), false);

  for (const auto &[border_x, border_y] : all_border_points) {
    for (size_t i = 0; i < local_components.size(); ++i) {
      if (merged[i] || local_components[i].empty()) {
        continue;
      }

      bool found_in_i = false;
      for (const auto &p : local_components[i]) {
        if (p.x == border_x && p.y == border_y) {
          found_in_i = true;
          break;
        }
      }

      if (found_in_i) {
        for (size_t j = 0; j < local_components.size(); ++j) {
          if (i == j || merged[j] || local_components[j].empty()) {
            continue;
          }

          if (HasBorderPoint(local_components[j], local_border_set)) {
            local_components[i].insert(local_components[i].end(), local_components[j].begin(),
                                       local_components[j].end());
            local_components[j].clear();
            merged[j] = true;
            changed = true;
          }
        }
      }
    }
  }

  if (changed) {
    auto it = std::remove_if(local_components.begin(), local_components.end(),
                             [](const std::vector<Point> &comp) { return comp.empty(); });
    local_components.erase(it, local_components.end());
  }
}

void GatherAllComponents(int rank, int size, std::vector<std::vector<Point>> &local_components,
                         std::vector<std::vector<Point>> &all_components) {
  if (rank == 0) {
    all_components = local_components;

    for (int i = 1; i < size; ++i) {
      int comp_count = 0;
      MPI_Recv(&comp_count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      for (int j = 0; j < comp_count; ++j) {
        int comp_size = 0;
        MPI_Recv(&comp_size, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (comp_size > 0) {
          std::vector<int> point_data(static_cast<size_t>(comp_size) * 2);
          MPI_Recv(point_data.data(), comp_size * 2, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

          std::vector<Point> component;
          component.reserve(static_cast<size_t>(comp_size));
          for (int k = 0; k < comp_size; ++k) {
            component.emplace_back(point_data[static_cast<size_t>(k) * 2],
                                   point_data[(static_cast<size_t>(k) * 2) + 1]);
          }
          all_components.push_back(component);
        }
      }
    }
  } else {
    int comp_count = static_cast<int>(local_components.size());
    MPI_Send(&comp_count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

    for (const auto &component : local_components) {
      int comp_size = static_cast<int>(component.size());
      MPI_Send(&comp_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

      if (comp_size > 0) {
        std::vector<int> point_data;
        point_data.reserve(static_cast<size_t>(comp_size) * 2);
        for (const auto &point : component) {
          point_data.push_back(point.x);
          point_data.push_back(point.y);
        }
        MPI_Send(point_data.data(), comp_size * 2, MPI_INT, 0, 0, MPI_COMM_WORLD);
      }
    }
  }
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
    local_data_.convex_hulls.clear();
    for (const auto &component : local_data_.components) {
      if (component.size() >= 3) {
        local_data_.convex_hulls.push_back(GrahamScan(component));
      } else if (!component.empty()) {
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

void KotelnikovaAConvexHullForBinImgMPI::BinarizeImageMpi() {
  const uint8_t threshold = 128;
  int total_pixels = local_data_.width * local_data_.height;

  if (rank_ == 0) {
    std::vector<int> counts(static_cast<size_t>(size_));
    std::vector<int> displs(static_cast<size_t>(size_));

    int base_count = total_pixels / size_;
    int remainder = total_pixels % size_;

    for (int i = 0; i < size_; ++i) {
      counts[static_cast<size_t>(i)] = base_count + (i < remainder ? 1 : 0);
      displs[static_cast<size_t>(i)] =
          (i == 0) ? 0 : displs[static_cast<size_t>(i - 1)] + counts[static_cast<size_t>(i - 1)];
    }

    std::vector<uint8_t> local_pixels(static_cast<size_t>(counts[0]));

    MPI_Scatterv(local_data_.pixels.data(), counts.data(), displs.data(), MPI_UINT8_T, local_pixels.data(), counts[0],
                 MPI_UINT8_T, 0, MPI_COMM_WORLD);

    for (auto &pixel : local_pixels) {
      pixel = (pixel > threshold) ? static_cast<uint8_t>(255) : static_cast<uint8_t>(0);
    }

    MPI_Gatherv(local_pixels.data(), counts[0], MPI_UINT8_T, local_data_.pixels.data(), counts.data(), displs.data(),
                MPI_UINT8_T, 0, MPI_COMM_WORLD);

    MPI_Bcast(local_data_.pixels.data(), total_pixels, MPI_UINT8_T, 0, MPI_COMM_WORLD);
  } else {
    int base_count = total_pixels / size_;
    int remainder = total_pixels % size_;
    int local_count = base_count + (rank_ < remainder ? 1 : 0);

    std::vector<uint8_t> local_pixels(static_cast<size_t>(local_count));

    MPI_Scatterv(nullptr, nullptr, nullptr, MPI_UINT8_T, local_pixels.data(), local_count, MPI_UINT8_T, 0,
                 MPI_COMM_WORLD);

    for (auto &pixel : local_pixels) {
      pixel = (pixel > threshold) ? static_cast<uint8_t>(255) : static_cast<uint8_t>(0);
    }

    MPI_Gatherv(local_pixels.data(), local_count, MPI_UINT8_T, nullptr, nullptr, nullptr, MPI_UINT8_T, 0,
                MPI_COMM_WORLD);

    MPI_Bcast(local_data_.pixels.data(), total_pixels, MPI_UINT8_T, 0, MPI_COMM_WORLD);
  }
}

void KotelnikovaAConvexHullForBinImgMPI::FindConnectedComponentsMpi() {
  int width = local_data_.width;
  int height = local_data_.height;

  int rows_per_proc = height / size_;
  int remainder = height % size_;

  int start_row = (rank_ * rows_per_proc) + std::min(rank_, remainder);
  int end_row = start_row + rows_per_proc + (rank_ < remainder ? 1 : 0);

  std::vector<std::vector<Point>> local_components;
  std::vector<Point> border_pixels;
  ProcessLocalRegion(start_row, end_row, width, local_data_, local_components, border_pixels);

  std::vector<std::vector<Point>> received_borders;
  ExchangeBorderPixels(size_, border_pixels, received_borders);

  MergeBorderComponents(local_components, received_borders, rank_, start_row, end_row);

  std::vector<std::vector<Point>> all_components;
  GatherAllComponents(rank_, size_, local_components, all_components);

  if (rank_ == 0) {
    local_data_.components = all_components;
  } else {
    local_data_.components.clear();
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
