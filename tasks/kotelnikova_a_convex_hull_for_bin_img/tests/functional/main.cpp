#include <gtest/gtest.h>

#include "kotelnikova_a_convex_hull_for_bin_img/common/include/common.hpp"
#include "kotelnikova_a_convex_hull_for_bin_img/mpi/include/ops_mpi.hpp"
#include "kotelnikova_a_convex_hull_for_bin_img/seq/include/ops_seq.hpp"
#include "util/include/func_test_util.hpp"
#include "util/include/util.hpp"

namespace kotelnikova_a_convex_hull_for_bin_img {

class KotelnikovaAFuncTestsProcesses3 : public ppc::util::BaseRunFuncTests<InType, OutType, TestType> {
 public:
  static std::string PrintTestParam(const TestType &test_param) {
    return std::to_string(std::get<0>(test_param)) + "_" + std::get<1>(test_param);
  }

 protected:
  void SetUp() override {}

  bool CheckTestOutputData(OutType &output_data) final {
    for (const auto &hull : output_data.convex_hulls) {
      if (hull.empty()) {
        return false;
      }

      for (size_t i = 0; i < hull.size(); ++i) {
        for (size_t j = i + 1; j < hull.size(); ++j) {
          if (hull[i] == hull[j]) {
            return false;
          }
        }
      }

      if (hull.size() >= 3) {
        for (size_t i = 0; i < hull.size(); ++i) {
          const Point &p0 = hull[i];
          const Point &p1 = hull[(i + 1) % hull.size()];
          const Point &p2 = hull[(i + 2) % hull.size()];

          int cross = (p1.x - p0.x) * (p2.y - p1.y) - (p1.y - p0.y) * (p2.x - p1.x);
          if (cross <= 0) {
            return false;
          }
        }
      }
    }
    return true;
  }

  InType GetTestInputData() final {
    InType data;

    auto test_param = std::get<2>(GetParam());
    int test_case = std::get<0>(test_param);

    data.width = 20 * test_case;
    data.height = 20 * test_case;
    data.pixels.resize(data.width * data.height, 0);

    switch (test_case) {
      case 1:
        createSquarePattern(data);
        break;
      case 2:
        createTrianglePattern(data);
        break;
      case 3:
        createCirclePattern(data);
        break;
      case 4:
        createMultipleComponents(data);
        break;
      case 5:
        createLinePattern(data);
        break;
      case 6:
        createPatternWithHole(data);
        break;
      case 7:
        createLShapePattern(data);
        break;
      default:
        createSquarePattern(data);
    }

    return data;
  }

 private:
  void createSquarePattern(ImageData &data) {
    int center_x = data.width / 2;
    int center_y = data.height / 2;
    int size = 6;

    for (int dy = -size / 2; dy <= size / 2; ++dy) {
      for (int dx = -size / 2; dx <= size / 2; ++dx) {
        int x = center_x + dx;
        int y = center_y + dy;
        if (x >= 0 && x < data.width && y >= 0 && y < data.height) {
          int idx = y * data.width + x;
          data.pixels[idx] = 255;
        }
      }
    }
  }

  void createTrianglePattern(ImageData &data) {
    int center_x = data.width / 2;
    int center_y = data.height / 2;
    int size = 8;

    for (int row = 0; row < size; ++row) {
      for (int col = 0; col <= row; ++col) {
        int px = center_x - size / 2 + col;
        int py = center_y - size / 2 + row;
        if (px >= 0 && px < data.width && py >= 0 && py < data.height) {
          int idx = py * data.width + px;
          data.pixels[idx] = 255;
        }
      }
    }
  }

  void createCirclePattern(ImageData &data) {
    int center_x = data.width / 2;
    int center_y = data.height / 2;
    int radius = 8;

    for (int dy = -radius; dy <= radius; ++dy) {
      for (int dx = -radius; dx <= radius; ++dx) {
        if (dx * dx + dy * dy <= radius * radius) {
          int px = center_x + dx;
          int py = center_y + dy;
          if (px >= 0 && px < data.width && py >= 0 && py < data.height) {
            int idx = py * data.width + px;
            data.pixels[idx] = 255;
          }
        }
      }
    }
  }

  void createMultipleComponents(ImageData &data) {
    createSquarePattern(data);

    int small_size = 3;

    for (int row = 0; row < small_size; ++row) {
      for (int col = 0; col < small_size; ++col) {
        int idx = row * data.width + col;
        data.pixels[idx] = 255;
      }
    }

    for (int row = 0; row < small_size; ++row) {
      for (int col = data.width - small_size; col < data.width; ++col) {
        int idx = row * data.width + col;
        data.pixels[idx] = 255;
      }
    }

    for (int row = data.height - small_size; row < data.height; ++row) {
      for (int col = 0; col < small_size; ++col) {
        int idx = row * data.width + col;
        data.pixels[idx] = 255;
      }
    }
  }

  void createLinePattern(ImageData &data) {
    int line_y = data.height / 2;
    for (int x = data.width / 4; x < 3 * data.width / 4; ++x) {
      int idx = line_y * data.width + x;
      data.pixels[idx] = 255;
    }

    int line_x = data.width / 2;
    for (int y = data.height / 4; y < 3 * data.height / 4; ++y) {
      int idx = y * data.width + line_x;
      data.pixels[idx] = 255;
    }
  }

  void createPatternWithHole(ImageData &data) {
    int center_x = data.width / 2;
    int center_y = data.height / 2;
    int outer_size = 10;
    int inner_size = 4;

    for (int dy = -outer_size / 2; dy <= outer_size / 2; ++dy) {
      for (int dx = -outer_size / 2; dx <= outer_size / 2; ++dx) {
        if (abs(dx) <= inner_size / 2 && abs(dy) <= inner_size / 2) {
          continue;
        }
        int px = center_x + dx;
        int py = center_y + dy;
        if (px >= 0 && px < data.width && py >= 0 && py < data.height) {
          int idx = py * data.width + px;
          data.pixels[idx] = 255;
        }
      }
    }
  }

  void createLShapePattern(ImageData &data) {
    int center_x = data.width / 2;
    int center_y = data.height / 2;
    int size = 8;

    for (int dy = -size / 2; dy <= size / 2; ++dy) {
      int x = center_x - size / 2;
      int px = x;
      int py = center_y + dy;
      if (px >= 0 && px < data.width && py >= 0 && py < data.height) {
        int idx = py * data.width + px;
        data.pixels[idx] = 255;
      }
    }

    for (int dx = -size / 2; dx <= size / 2; ++dx) {
      int y_pos = center_y + size / 2;
      int px = center_x + dx;
      int py = y_pos;
      if (px >= 0 && px < data.width && py >= 0 && py < data.height) {
        int idx = py * data.width + px;
        data.pixels[idx] = 255;
      }
    }
  }
};

namespace {

TEST_P(KotelnikovaAFuncTestsProcesses3, MatmulFromPic) {
  ExecuteTest(GetParam());
}

const std::array<TestType, 7> kTestParam = {std::make_tuple(1, "square"), std::make_tuple(2, "triangle"),
                                            std::make_tuple(3, "circle"), std::make_tuple(4, "multiple"),
                                            std::make_tuple(5, "lines"),  std::make_tuple(6, "with_hole"),
                                            std::make_tuple(7, "L_shape")};

const auto kTestTasksList = std::tuple_cat(ppc::util::AddFuncTask<KotelnikovaAConvexHullForBinImgMPI, InType>(
                                               kTestParam, PPC_SETTINGS_kotelnikova_a_convex_hull_for_bin_img),
                                           ppc::util::AddFuncTask<KotelnikovaAConvexHullForBinImgSEQ, InType>(
                                               kTestParam, PPC_SETTINGS_kotelnikova_a_convex_hull_for_bin_img));

const auto kGtestValues = ppc::util::ExpandToValues(kTestTasksList);

const auto kPerfTestName = KotelnikovaAFuncTestsProcesses3::PrintFuncTestName<KotelnikovaAFuncTestsProcesses3>;

INSTANTIATE_TEST_SUITE_P(PicMatrixTests, KotelnikovaAFuncTestsProcesses3, kGtestValues, kPerfTestName);

}  // namespace

}  // namespace kotelnikova_a_convex_hull_for_bin_img
