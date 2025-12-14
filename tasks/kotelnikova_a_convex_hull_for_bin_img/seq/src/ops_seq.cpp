#include "kotelnikova_a_convex_hull_for_bin_img/seq/include/ops_seq.hpp"

#include <algorithm>
#include <cmath>
#include <queue>
#include <set>

namespace kotelnikova_a_convex_hull_for_bin_img {

static int cross(const Point &O, const Point &A, const Point &B) {
  return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

KotelnikovaAConvexHullForBinImgSEQ::KotelnikovaAConvexHullForBinImgSEQ(const InType &in) {
  SetTypeOfTask(GetStaticTypeOfTask());
  GetInput() = in;
  processed_data_ = in;
}

bool KotelnikovaAConvexHullForBinImgSEQ::ValidationImpl() {
  return GetInput().width > 0 && GetInput().height > 0 && !GetInput().pixels.empty() &&
         GetInput().pixels.size() == static_cast<size_t>(GetInput().width * GetInput().height);
}

bool KotelnikovaAConvexHullForBinImgSEQ::PreProcessingImpl() {
  binarizeImage();
  return true;
}

bool KotelnikovaAConvexHullForBinImgSEQ::RunImpl() {
  findConnectedComponents();
  processed_data_.convex_hulls.clear();

  for (const auto &component : processed_data_.components) {
    if (component.size() >= 3) {
      processed_data_.convex_hulls.push_back(grahamScan(component));
    } else if (component.size() > 0) {
      processed_data_.convex_hulls.push_back(component);
    }
  }

  GetOutput() = processed_data_;
  return true;
}

bool KotelnikovaAConvexHullForBinImgSEQ::PostProcessingImpl() {
  return true;
}

void KotelnikovaAConvexHullForBinImgSEQ::binarizeImage() {
  const uint8_t threshold = 128;
  for (auto &pixel : processed_data_.pixels) {
    pixel = (pixel > threshold) ? 255 : 0;
  }
}

void KotelnikovaAConvexHullForBinImgSEQ::findConnectedComponents() {
  int width = processed_data_.width;
  int height = processed_data_.height;
  int total_pixels = width * height;
  std::vector<bool> visited(static_cast<size_t>(total_pixels), false);
  processed_data_.components.clear();

  std::vector<std::pair<int, int>> directions = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int idx = y * width + x;
      if (processed_data_.pixels[static_cast<size_t>(idx)] == 255 && !visited[static_cast<size_t>(idx)]) {
        std::vector<Point> component;
        std::queue<Point> q;
        q.push(Point(x, y));
        visited[static_cast<size_t>(idx)] = true;

        while (!q.empty()) {
          Point p = q.front();
          q.pop();
          component.push_back(p);

          for (const auto &dir : directions) {
            int nx = p.x + dir.first;
            int ny = p.y + dir.second;

            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
              int nidx = ny * width + nx;
              if (processed_data_.pixels[static_cast<size_t>(nidx)] == 255 && !visited[static_cast<size_t>(nidx)]) {
                visited[static_cast<size_t>(nidx)] = true;
                q.push(Point(nx, ny));
              }
            }
          }
        }

        if (!component.empty()) {
          processed_data_.components.push_back(component);
        }
      }
    }
  }
}

std::vector<Point> KotelnikovaAConvexHullForBinImgSEQ::grahamScan(const std::vector<Point> &points) {
  if (points.size() <= 3) {
    return points;
  }

  std::vector<Point> pts = points;
  size_t n = pts.size();

  size_t min_idx = 0;
  for (size_t i = 1; i < n; ++i) {
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
  for (size_t i = 0; i < n; ++i) {
    while (hull.size() >= 2 && cross(hull[hull.size() - 2], hull.back(), pts[i]) <= 0) {
      hull.pop_back();
    }
    hull.push_back(pts[i]);
  }

  return hull;
}
}  // namespace kotelnikova_a_convex_hull_for_bin_img
