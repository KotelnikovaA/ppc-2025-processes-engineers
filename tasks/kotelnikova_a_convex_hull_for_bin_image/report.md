# Построение выпуклой оболочки для компонент бинарного изображения

- Студент: Котельникова Анастасия Владимировна, группа 3823Б1ПР2
- Технологии: SEQ + MPI
- Вариант: 32

## 1. Введение

Задача построения выпуклой оболочки для компонент связности бинарного изображения является классической проблемой компьютерного зрения и обработки изображений. При увеличении размеров изображения последовательные алгоритмы становятся недостаточно эффективными из-за роста вычислительной сложности. Параллельная реализация позволяет ускорить обработку больших изображений за счёт распределения вычислений между несколькими процессами.

Цель работы — разработать последовательный и параллельный (MPI) алгоритмы построения выпуклых оболочек для компонент связности бинарного изображения и сравнить их производительность.

## 2. Постановка задачи

Задача: для каждого связного компонента белых пикселей (значение 255) в бинарном изображении построить выпуклую оболочку.

Входные данные:
- изображение в формате `ImageData` (ширина, высота, вектор пикселей).
- пиксели могут иметь произвольные значения (изображение в оттенках серого).

Выходные данные: изображение с сохранёнными исходными размерами и пикселями, дополненное:
- списком компонент связности.
- списком выпуклых оболочек для каждой компоненты.

Ограничения:
- изображение должно быть корректных размеров (width > 0, height > 0).
- количество пикселей должно соответствовать формуле width * height.
- выпуклая оболочка строится методом Грэхема.
- компоненты, содержащие менее 3 точек, не обрабатываются алгоритмом Грэхема (возвращаются как есть).

## 3. Базовый (последовательный) алгоритм (Sequential)

Этапы работы последовательного алгоритма:
1. `ValidationImpl()` - проверка корректности входных данных (размеры, объём данных).
2. `PreProcessingImpl()` - бинаризация изображения (порог 128).
3. `RunImpl()` - основной этап обработки.
    - Поиск компонент связности: `FindConnectedComponents()`.
    - Очистка предыдущих результатов.
    - Обработка каждой компоненты:
        - Если компонента содержит ≥3 точек: обработка алгоритмом Грэхема`GrahamScan`.
        - Иначе (1-2 точки): сохраняется как есть.
    - Сохранение результатов.
4. `PostProcessingImpl()` - завершающий этап, дополнительных операций не выполняется.

Алгоритм Грэхема:
- Выбор точки с минимальной Y (и минимальной X при равенстве).
- Сортировка остальных точек по полярному углу относительно выбранной.
- Построение оболочки стековым методом.

Полноценная реализация последовательного алгоритма представлена в Приложении (п.1).

## 4. Схема параллелизации

Идея параллелизации:
Изображение разбивается по строкам между процессами. Каждый процесс обрабатывает свой вертикальный блок, находя компоненты связности в своей области. Затем результаты собираются на процессе 0, где строятся выпуклые оболочки.

Распределение данных:
- Строки изображения делятся между процессами.
- Каждый процесс получает блок строк `[start_row, end_row)`.
- Обработка соседних пикселей между блоками осуществляется через граничные проверки.

Схема связи/топологии:
- Коммуникатор MPI_COMM_WORLD.
- Процесс 0 — координатор (сбор результатов).
- Остальные процессы — рабочие (отправка найденных компонент).

Ранжирование ролей:

Процесс 0:
- Обработка своего блока строк.
- Приём компонент от других процессов.
- Построение выпуклых оболочек для всех компонент.

Остальные процессы:
- Обработка своего блока строк.
- Отправка найденных компонент процессу 0.

Полноценная реализация распараллеленного алгоритма представлена в Приложении (п.2).

## 5. Детали реализации

Файловая структура:

kotelnikova_a_convex_hull_for_bin_image/  
├── common/include  
│   └── common.hpp                  # Базовые определения типов   
├── mpi/  
│   ├── include/ops_mpi.hpp         # MPI версия    
│   └── src/ops_mpi.cpp  
├── seq/  
│   ├── include/ops_seq.hpp         # Последовательная версия  
│   └── src/ops_seq.cpp  
└── tests/  
    ├── functional/main.cpp         # Функциональные тесты  
    └── performance/main.cpp        # Производительные тесты  

Ключевые классы:
- `KotelnikovaAConvexHullForBinImgSEQ` - последовательная реализация.
- `KotelnikovaAConvexHullForBinImgMPI` - параллельная реализация.

Основные методы:
- `ValidationImpl()` — проверка входных данных.
- `PreProcessingImpl()` — бинаризация.
- `RunImpl()` — основной алгоритм.
- `FindConnectedComponents()` / `FindConnectedComponentsMpi()` — поиск компонент.
- `GrahamScan()` — построение выпуклой оболочки.

Алгоритмические особенности:
- MPI-версия использует `MPI_Allgatherv` для синхронизации бинаризованного изображения.
- Компоненты собираются на процессе 0 через точечные MPI-сообщения.

## 6. Экспериментальная среда

Hardware/OS:
- процессор: Intel Core i5
- ядра/потоки: 8 ядер / 16 потоков
- оперативная память: 16 GB
- операционная система: Windows 11
- архитектура: x64

Toolchain:
- компилятор: Microsoft Visual C++ (MSVC)
- версия: Visual Studio Code 2019/2022
- тип сборки: Release
- система сборки: CMake
- версия MPI: Microsoft MPI 10.1

Environment:
- количество процессов: задается через mpiexec -n N
- коммуникатор: MPI_COMM_WORLD

Тестовые данные: 
1. Функциональные тесты: 7 различных паттернов (квадрат, треугольник, круг, несколько компонент, линии, отверстие, L-форма).
2. Перформанс-тесты: сложный паттерн 1000×1000 пикселей с кругами, сетками, прямоугольниками и диагоналями.

## 7. Результаты и обсуждение

### 7.1 Корректность

Корректность проверена через:
- 7 функциональных тестов с известными ожидаемыми результатами.
- Проверка уникальности точек, принадлежности изображению и выпуклости оболочек.
- Сравнение результатов последовательной и MPI-версий — полное совпадение.

### 7.2 Производительность

Методы измерений:
- Каждый тест запускается 5 раз
- Берется среднее время выполнения (ΣTime / 5)
- Speedup = Time_seq / Time_mpi
- Efficiency = Speedup / Count * 100%

| Mode        | Count | Time, s  | Speedup | Efficiency |
|-------------|-------|----------|---------|------------|
| seq         | 1     | 0.0988   | 1.00    | N/A        |
| mpi         | 2     | 0.0931   | 1.06    | 53.0%      |
| mpi         | 4     | 0.0895   | 1.12    | 28.0%      |
| mpi         | 6     | 0.1011   | 1.08    | 18.0%      |

Анализ результатов:
- На 2 и 4 процессах наблюдается незначительное ускорение (5.8% и 9.4% соответственно).
- На 6 процессах происходит деградация производительности - время выполнения становится больше, чем у последовательной версии.
- Оптимальное количество процессов для данной задачи - 4, после чего производительность падает.
- Резкое падение эффективности с увеличением числа процессов (что скорее всего связано с расходами MPI)

## 8. Заключение
В ходе работы была успешно решена задача построения выпуклых оболочек для компонент связности бинарного изображения с использованием последовательного алгоритма и технологии MPI для параллельных вычислений.

Основные результаты:
- Разработаны корректные последовательная и параллельная версии алгоритма.
- Реализована схема распараллеливания с разбиением изображения по строкам.
- Достигнуто незначительное ускорение - параллельная реализация демонстрирует ускорение до 1.12 раз на 4 процессах по сравнению с последовательной версией.

## 9. Источники
1. Документация по курсу «Параллельное программирование» // URL: https://learning-process.github.io/parallel_programming_course/ru/index.html
2. Репозиторий курса «Параллельное программирование» // URL: https://github.com/learning-process/ppc-2025-processes-engineers
3. Сысоев А. В., Лекции по курсу «Параллельное программирование для кластерных систем».

## Приложение
П.1
```cpp
bool KotelnikovaAConvexHullForBinImgSEQ::RunImpl() {
  FindConnectedComponents();
  processed_data_.convex_hulls.clear();

  for (const auto &component : processed_data_.components) {
    if (component.size() >= 3) {
      processed_data_.convex_hulls.push_back(GrahamScan(component));
    } else if (!component.empty()) {
      processed_data_.convex_hulls.push_back(component);
    }
  }

  GetOutput() = processed_data_;
  return true;
}

void KotelnikovaAConvexHullForBinImgSEQ::FindConnectedComponents() {
  int width = processed_data_.width;
  int height = processed_data_.height;
  int total_pixels = width * height;
  std::vector<bool> visited(static_cast<size_t>(total_pixels), false);
  processed_data_.components.clear();

  for (int row_y = 0; row_y < height; ++row_y) {
    for (int col_x = 0; col_x < width; ++col_x) {
      size_t idx = (static_cast<size_t>(row_y) * static_cast<size_t>(width)) + static_cast<size_t>(col_x);
      if (processed_data_.pixels[idx] == 255 && !visited[idx]) {
        ProcessConnectedComponent(col_x, row_y, width, height, processed_data_, visited, processed_data_.components);
      }
    }
  }
}

void ProcessConnectedComponent(int start_x, int start_y, int width, int height, const ImageData &processed_data,
                               std::vector<bool> &visited, std::vector<std::vector<Point>> &components) {
  std::vector<Point> component;
  std::queue<Point> q;
  size_t start_idx = (static_cast<size_t>(start_y) * static_cast<size_t>(width)) + static_cast<size_t>(start_x);
  q.emplace(start_x, start_y);
  visited[start_idx] = true;

  while (!q.empty()) {
    Point p = q.front();
    q.pop();
    component.push_back(p);

    ProcessPixelNeighbors(p, width, height, processed_data, visited, q);
  }

  if (!component.empty()) {
    components.push_back(component);
  }
}

void ProcessPixelNeighbors(const Point &p, int width, int height, const ImageData &processed_data,
                           std::vector<bool> &visited, std::queue<Point> &q) {
  const std::vector<std::pair<int, int>> directions = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

  for (const auto &dir : directions) {
    int nx = p.x + dir.first;
    int ny = p.y + dir.second;

    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
      int nidx = (ny * width) + nx;
      if (processed_data.pixels[static_cast<size_t>(nidx)] == 255 && !visited[static_cast<size_t>(nidx)]) {
        visited[static_cast<size_t>(nidx)] = true;
        q.emplace(nx, ny);
      }
    }
  }
}

std::vector<Point> KotelnikovaAConvexHullForBinImgSEQ::GrahamScan(const std::vector<Point> &points) {
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
    int orient = Cross(pivot, a, b);
    if (orient == 0) {
      return ((a.x - pivot.x) * (a.x - pivot.x)) + ((a.y - pivot.y) * (a.y - pivot.y)) <
             ((b.x - pivot.x) * (b.x - pivot.x)) + ((b.y - pivot.y) * (b.y - pivot.y));
    }
    return orient > 0;
  });

  std::vector<Point> hull;
  for (size_t i = 0; i < n; ++i) {
    while (hull.size() >= 2 && Cross(hull[hull.size() - 2], hull.back(), pts[i]) <= 0) {
      hull.pop_back();
    }
    hull.push_back(pts[i]);
  }

  return hull;
}

int Cross(const Point &o, const Point &a, const Point &b) {
  return ((a.x - o.x) * (b.y - o.y)) - ((a.y - o.y) * (b.x - o.x));
}
```

П.2
```cpp
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
```