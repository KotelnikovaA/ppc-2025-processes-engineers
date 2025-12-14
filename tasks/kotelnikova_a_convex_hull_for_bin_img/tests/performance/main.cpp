#include <gtest/gtest.h>

#include <cmath>
#include <numbers>
#include <set>

#include "kotelnikova_a_convex_hull_for_bin_img/common/include/common.hpp"
#include "kotelnikova_a_convex_hull_for_bin_img/mpi/include/ops_mpi.hpp"
#include "kotelnikova_a_convex_hull_for_bin_img/seq/include/ops_seq.hpp"
#include "util/include/perf_test_util.hpp"

namespace kotelnikova_a_convex_hull_for_bin_img {

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

class KotelnikovaARunPerfTestProcesses3 : public ppc::util::BaseRunPerfTests<InType, OutType> {
 protected:
  void SetUp() override {}

  bool CheckTestOutputData(OutType &output_data) final {
    if (output_data.width <= 0 || output_data.height <= 0) {
      return false;
    }

    for (const auto &hull : output_data.convex_hulls) {
      if (hull.empty()) {
        return false;
      }
      std::set<Point> unique_points;
      for (const auto &point : hull) {
        if (!unique_points.insert(point).second) {
          return false;
        }
      }

      for (const auto &point : hull) {
        if (point.x < 0 || point.x >= output_data.width || point.y < 0 || point.y >= output_data.height) {
          return false;
        }
      }

      if (hull.size() >= 3) {
        bool all_positive = true;
        bool all_negative = true;

        for (size_t i = 0; i < hull.size(); ++i) {
          const Point &p0 = hull[i];
          const Point &p1 = hull[(i + 1) % hull.size()];
          const Point &p2 = hull[(i + 2) % hull.size()];

          int cross = (p1.x - p0.x) * (p2.y - p1.y) - (p1.y - p0.y) * (p2.x - p1.x);

          if (cross > 0) {
            all_negative = false;
          }
          if (cross < 0) {
            all_positive = false;
          }
        }

        if (!(all_positive || all_negative)) {
          return false;
        }
      }
      if (hull.size() == 2) {
        if (hull[0] == hull[1]) {
          return false;
        }
      }
    }
    return true;
  }

  InType GetTestInputData() final {
    InType data;

    int base_size = 1000;

    data.width = base_size;
    data.height = base_size;
    data.pixels.resize(static_cast<size_t>(data.width) * data.height, 0);

    createHeavyTestPattern(data);

    return data;
  }

 private:
  void createHeavyTestPattern(ImageData &data) {
    std::fill(data.pixels.begin(), data.pixels.end(), 0);

    int center_x = data.width / 2;
    int center_y = data.height / 2;
    int big_radius = std::min(data.width, data.height) / 3;

    for (int y = -big_radius; y <= big_radius; ++y) {
      for (int x = -big_radius; x <= big_radius; ++x) {
        if (x * x + y * y <= big_radius * big_radius) {
          int px = center_x + x;
          int py = center_y + y;
          if (px >= 0 && px < data.width && py >= 0 && py < data.height) {
            size_t idx = static_cast<size_t>(py) * data.width + px;
            data.pixels[idx] = 255;
          }
        }
      }
    }

    int small_radius = data.width / 20;
    int circle_count = 16;

    for (int i = 0; i < circle_count; ++i) {
      double angle = 2.0 * M_PI * i / circle_count;
      int circ_x = center_x + static_cast<int>(big_radius * 1.7 * cos(angle));
      int circ_y = center_y + static_cast<int>(big_radius * 1.7 * sin(angle));

      for (int y = -small_radius; y <= small_radius; ++y) {
        for (int x = -small_radius; x <= small_radius; ++x) {
          if (x * x + y * y <= small_radius * small_radius) {
            int px = circ_x + x;
            int py = circ_y + y;
            if (px >= 0 && px < data.width && py >= 0 && py < data.height) {
              size_t idx = static_cast<size_t>(py) * data.width + px;
              data.pixels[idx] = 255;
            }
          }
        }
      }
    }

    int grid_cells = 20;
    int cell_width = data.width / grid_cells;
    int cell_height = data.height / grid_cells;

    for (int i = 1; i < grid_cells; ++i) {
      int line_y = i * cell_height;
      int line_thickness = 2;

      for (int y = line_y - line_thickness; y <= line_y + line_thickness; ++y) {
        if (y >= 0 && y < data.height) {
          for (int x = 0; x < data.width; ++x) {
            size_t idx = static_cast<size_t>(y) * data.width + x;
            data.pixels[idx] = 255;
          }
        }
      }
    }

    for (int i = 1; i < grid_cells; ++i) {
      int line_x = i * cell_width;
      int line_thickness = 2;

      for (int x = line_x - line_thickness; x <= line_x + line_thickness; ++x) {
        if (x >= 0 && x < data.width) {
          for (int y = 0; y < data.height; ++y) {
            if (y % 4 < 2) {
              size_t idx = static_cast<size_t>(y) * data.width + x;
              data.pixels[idx] = 255;
            }
          }
        }
      }
    }

    int rect_count = 8;
    for (int i = 0; i < rect_count; ++i) {
      int rect_width = data.width / (rect_count / 2 + 2);
      int rect_height = data.height / (rect_count / 2 + 2);

      int x1 = (i * 2 * data.width) / rect_count;
      int y1 = (i * 2 * data.height) / rect_count;
      int x2 = x1 + rect_width * (i % 2 + 1);
      int y2 = y1 + rect_height * (i % 3 + 1);

      x1 = std::max(0, x1);
      y1 = std::max(0, y1);
      x2 = std::min(data.width - 1, x2);
      y2 = std::min(data.height - 1, y2);

      for (int y = y1; y <= y2; ++y) {
        for (int x = x1; x <= x2; ++x) {
          size_t idx = static_cast<size_t>(y) * data.width + x;
          data.pixels[idx] = 255;
        }
      }
    }

    int diagonal_count = 15;
    for (int i = 0; i < diagonal_count; ++i) {
      int start_x = (i * data.width) / diagonal_count;
      int start_y = 0;
      int end_x = data.width;
      int end_y = (i * data.height) / diagonal_count;

      drawThickLine(data, start_x, start_y, end_x, end_y, 3);
    }

    for (int i = 0; i < diagonal_count; ++i) {
      int start_x = 0;
      int start_y = (i * data.height) / diagonal_count;
      int end_x = data.width;
      int end_y = (i * data.height) / diagonal_count;

      drawThickLine(data, start_x, start_y, end_x, end_y, 3);
    }
  }

  void drawThickLine(ImageData &data, int x1, int y1, int x2, int y2, int thickness) {
    for (int t = -thickness / 2; t <= thickness / 2; ++t) {
      drawLine(data, x1, y1 + t, x2, y2 + t);
      drawLine(data, x1 + t, y1, x2 + t, y2);
    }
  }

  void drawLine(ImageData &data, int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
      if (x1 >= 0 && x1 < data.width && y1 >= 0 && y1 < data.height) {
        size_t idx = static_cast<size_t>(y1) * data.width + x1;
        data.pixels[idx] = 255;
      }

      if (x1 == x2 && y1 == y2) {
        break;
      }

      int e2 = 2 * err;
      if (e2 > -dy) {
        err -= dy;
        x1 += sx;
      }
      if (e2 < dx) {
        err += dx;
        y1 += sy;
      }
    }
  }
};

TEST_P(KotelnikovaARunPerfTestProcesses3, RunPerfModes) {
  ExecuteTest(GetParam());
}

const auto kAllPerfTasks =
    ppc::util::MakeAllPerfTasks<InType, KotelnikovaAConvexHullForBinImgMPI, KotelnikovaAConvexHullForBinImgSEQ>(
        PPC_SETTINGS_kotelnikova_a_convex_hull_for_bin_img);

const auto kGtestValues = ppc::util::TupleToGTestValues(kAllPerfTasks);

const auto kPerfTestName = KotelnikovaARunPerfTestProcesses3::CustomPerfTestName;

INSTANTIATE_TEST_SUITE_P(RunModeTests, KotelnikovaARunPerfTestProcesses3, kGtestValues, kPerfTestName);

}  // namespace kotelnikova_a_convex_hull_for_bin_img
