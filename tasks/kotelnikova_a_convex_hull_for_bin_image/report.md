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
- количество пикселей должно соответствовать формуле `width * height`.
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
- Процесс 0 выполняет бинаризацию всего изображения.
- Изображение разбивается по строкам между процессами с помощью MPI_Scatterv.
- Каждый процесс получает блок строк `[start_row, end_row)`.
- Для обработки компонент, пересекающих границы блоков, используется расширенная область с обменом граничными строками между соседними процессами.

Схема связи/топологии:
- Коммуникатор MPI_COMM_WORLD.
- Процесс 0 — координатор (распределение данных, сбор результатов, построение выпуклых оболочек).
- Все процессы — рабочие (поиск компонент связности и построение выпуклых оболочек для своих компонент).

Ранжирование ролей:

Процесс 0:
- Бинаризация всего изображения.
- Распределение данных по процессам (MPI_Scatterv).
- Обработка своего блока строк.
- Приём выпуклых оболочек от других процессов.
- Построение выпуклых оболочек для всех компонент.

Остальные процессы:
- Получение своего блока данных.
- Обработка своего блока строк.
- Отправка выпуклых оболочек процессу 0.

Ключевые особенности:
- Асинхронные вычисления: Каждый процесс независимо находит компоненты связности и строит выпуклые оболочки для своего блока.
- Обработка граничных компонент: Используется расширенная область (+1 строка сверху и снизу) и обмен граничными строками между соседними процессами.
- Эффективная коммуникация: Используется MPI_Scatterv для распределения данных и MPI_Gather для сбора результатов.
- Балансировка нагрузки: Распределение строк происходит с учетом остатка для равномерной загрузки процессов.

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
- `PostProcessingImpl` — не требуется.

Основные методы SEQ реализации:
- `FindConnectedComponents()` — поиск компонент.
- `GrahamScan()` — построение выпуклой оболочки.

Основные методы MPI реализации:
- `ScatterDataAndDistributeWork()` — распределение данных между процессами.
- `ExchangeBoundaryRows()` — обмен граничными строками между процессами.
- `ProcessExtendedRegion()` — обработка расширенной области с граничными строками.
- `ProcessExtendedNeighbors()` — обработка соседей в расширенной области.
- `FilterLocalComponents()` — фильтрация компонент, принадлежащих текущему процессу.
- `GatherConvexHullsToRank0()` — сбор выпуклых оболочек на процессе 0.
- `ReceiveHullsFromProcess()` / `SendHullsToRank0()` — приём/отправка выпуклых оболочек.

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
- 10 функциональных тестов с известными ожидаемыми результатами.
- Проверка уникальности точек, принадлежности изображению и выпуклости оболочек.
- Сравнение результатов последовательной и MPI-версий — полное совпадение.

### 7.2 Производительность

Методы измерений:
- Каждый тест запускается 5 раз
- Берется среднее время выполнения (ΣTime / 5)
- Speedup = Time_seq / Time_mpi
- Efficiency = Speedup / Count * 100%

| Mode        | Count | Time, s      | Speedup | Efficiency |
|-------------|-------|--------------|---------|------------|
| seq         | 1     | 0.0772740961 | 1.00    | N/A        |
| mpi         | 2     | 0.0449410556 | 1.72    | 86.0%      |
| mpi         | 4     | 0.0251098025 | 3.08    | 77.0%      |
| mpi         | 6     | 0.0228762227 | 3.38    | 56.3%      |

Анализ результатов:
- Значительное ускорение: Параллельная реализация демонстрирует существенное ускорение по сравнению с последовательной версией уже на 4ех процессах.
- Высокая эффективность на малом числе процессов:
  - 86.0% эффективности на 2 процессах - отличный результат, близкий к идеальному линейному ускорению
  - 77.0% эффективности на 4 процессах - хорошая эффективность при существенном ускорении
- С увеличением числа процессов эффективность снижается (56.3% на 6 процессах). Это объясняется ростом накладных расходов
- 4 процесса представляют оптимальный баланс между ускорением (3.08x) и эффективностью (77.0%). Дальнейшее увеличение числа процессов приводит к падению эффективности при незначительном приросте ускорения

## 8. Заключение
В ходе работы была успешно решена задача построения выпуклых оболочек для компонент связности бинарного изображения с использованием последовательного алгоритма и технологии MPI для параллельных вычислений.

Основные результаты:
- Разработаны корректные последовательная и параллельная версии алгоритма.
- Реализована схема распараллеливания с разбиением изображения по строкам.
- Достигнуто значительное ускорение - параллельная реализация демонстрирует ускорение до 3.38 раз на 6 процессах по сравнению с последовательной версией.
- Достигнуто хорошее значение эффективности - параллельная реализация демонстрирует эффективность до 86% на 2 процессах .

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
bool KotelnikovaAConvexHullForBinImgMPI::RunImpl() {
  FindConnectedComponentsMpi();
  ProcessComponentsAndComputeHulls();
  GatherConvexHullsToRank0();
  GetOutput() = local_data_;
  return true;
}


void KotelnikovaAConvexHullForBinImgMPI::FindConnectedComponentsMpi() {
  int width = local_data_.width;
  int height = local_data_.height;
  int local_rows = end_row_ - start_row_;

  int extended_start_row = std::max(0, start_row_ - 1);
  int extended_end_row = std::min(height, end_row_ + 1);
  int extended_local_rows = extended_end_row - extended_start_row;

  std::vector<uint8_t> extended_pixels(static_cast<size_t>(extended_local_rows) * width);

  for (int row = 0; row < local_rows; ++row) {
    int global_row = start_row_ + row;
    int ext_row = global_row - extended_start_row;
    for (int col = 0; col < width; ++col) {
      size_t local_idx = (static_cast<size_t>(row) * static_cast<size_t>(width)) + static_cast<size_t>(col);
      size_t ext_idx = (static_cast<size_t>(ext_row) * static_cast<size_t>(width)) + static_cast<size_t>(col);
      extended_pixels[ext_idx] = local_data_.pixels[local_idx];
    }
  }

  ExchangeBoundaryRows(width, local_rows, extended_start_row, extended_local_rows, extended_pixels);

  std::vector<bool> visited_extended(static_cast<size_t>(extended_local_rows) * width, false);
  std::vector<std::vector<Point>> all_components;

  ProcessExtendedRegion(width, extended_start_row, extended_local_rows, extended_pixels, visited_extended,
                        all_components);

  FilterLocalComponents(all_components);
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

void KotelnikovaAConvexHullForBinImgMPI::GatherConvexHullsToRank0() {
  std::vector<int> hull_counts(size_, 0);
  int local_hull_count = static_cast<int>(local_data_.convex_hulls.size());
  MPI_Gather(&local_hull_count, 1, MPI_INT, hull_counts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

  if (rank_ == 0) {
    std::vector<std::vector<Point>> rank0_hulls = local_data_.convex_hulls;
    local_data_.convex_hulls.clear();

    for (int i = 1; i < size_; ++i) {
      ReceiveHullsFromProcess(i, hull_counts[i]);
    }

    for (const auto &hull : rank0_hulls) {
      local_data_.convex_hulls.push_back(hull);
    }
  } else {
    SendHullsToRank0();
  }
}
```