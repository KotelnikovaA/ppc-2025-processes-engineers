# Передача от всех одному (reduce)

- Студент: Котельникова Анастасия Владимировна, группа 3823Б1ПР2
- Технологии: SEQ + MPI
- Вариант: 2

## 1. Введение

Задача передачи данных от всех процессов одному (операция Reduce) является одной из фундаментальных операций в параллельном программировании. Она позволяет агрегировать данные, распределённые между процессами, в единый результат на одном процессе (корневом). 

Цель работы — сравнить производительность последовательной и параллельной реализаций операции Reduce для различных типов данных (int, float, double).

## 2. Постановка задачи

Задача: Реализовать операцию Reduce (конкретно суммирование) для векторов числовых данных.

Входные данные: вектор типа `std::vector<int>`, `std::vector<float>` или `std::vector<double>`.
Выходные данные: вектор того же типа и размера, содержащий сумму соответствующих элементов всех процессов

Ограничения программы:
- все процессы должны иметь векторы одинакового размера
- поддерживается только операция суммирования (`MPI_SUM`).
- реализация должна корректно работать для пустых векторов

## 3. Базовый (последовательный) алгоритм (Sequential)

Последовательная реализация выполняет простую копию входного вектора в выходной, так как в последовательном режиме операция Reduce сводится к передаче данных самому себе.

Этапы работы:
1. `ValidationImpl()` — проверка, что входные данные соответствуют одному из поддерживаемых типов.
2. `PreProcessingImpl()` — дополнительных действий не требуется.
3. `RunImpl()` — копирование данных из входного вектора в выходной с помощью std::memcpy.
4. `PostProcessingImpl()` — завершающий этап без изменений.

Полноценная реализация последовательного алгоритма представлена в Приложении (п.1).

## 4. Схема параллелизации

Идея параллелизации:
Используется алгоритм бинарного дерева для выполнения операции Reduce. Каждый процесс суммирует данные от своих «потомков» и передаёт результат «родителю». Корневой процесс (ранг 0) аккумулирует финальный результат.

Распределение данных:
- Все процессы имеют локальную копию входного вектора.
- Размер вектора одинаков на всех процессах.

Схема связи/топологии:
- Топология: бинарное дерево, построенное на основе рангов процессов.
- Коммуникационные операции: MPI_Send и MPI_Recv для попарного обмена данными между процессами на каждом уровне дерева.

Ранжирование ролей:
- Все процессы равноправно участвуют в алгоритме дерева.
- Корневой процесс (ранг 0) является получателем финального результата.

Алгоритм обработки границ:
- На каждом уровне дерева процессы объединяются в пары (ранг `i` с рангом `i ^ (1 << level)`).
- Процесс с меньшим рангом в паре получает данные от партнёра и суммирует их со своими.
- Процесс с большим рангом отправляет свои данные и завершает участие.

Полноценная реализация распараллеленного алгоритма представлена в Приложении (п.2).

## 5. Детали реализации

Файловая структура:

kotelnikova_a_from_all_to_one/  
├── common/include  
│   └── common.hpp              # Базовые определения типов  
├── mpi/  
│   ├── include/ops_mpi.hpp     # MPI-версия Reduce  
│   └── src/ops_mpi.cpp  
├── seq/  
│   ├── include/ops_seq.hpp     # Последовательная версия  
│   └── src/ops_seq.cpp  
└── tests/  
    ├── functional/main.cpp     # Функциональные тесты  
    └── performance/main.cpp    # Тесты производительности  

Ключевые классы:
- `KotelnikovaAFromAllToOneSEQ` - последовательная реализация.
- `KotelnikovaAFromAllToOneMPI` - параллельная реализация.

Основные методы: 
- `ValidationImpl()` - проверка типа входных данных;
- `PreProcessingImpl()` - подготовительный этап (в MPI-версии инициализация выходного вектора нулями на корневом процессе).;
- `RunImpl()` - основной алгоритм (копирование или дерево Reduce);
- `PostProcessingImpl()` - завершающий этап;

Алгоритмические особенности:
- Поддержка трёх типов данных: `int`, `float`, `double`.
- Обработка пустых векторов (пропуск коммуникаций).
- Использование std::variant для универсального хранения векторов.

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
1. Функциональные тесты: 
   - 16 тестовых случаев на разных типах вхожных данных, а так же на векторах разной размерности.
2. Перформанс тесты:
   - вектор размера `4000000` элелентов типа `double` (так как данный тип самый ресурсоемкий)

## 7. Результаты и обсуждение

### 7.1 Корректность

Корректность проверена 16 функциональными тестами, охватывающими:  
- Положительные, отрицательные и смешанные значения.  
- Типы данных: int, float, double.  
- Крайние случаи: пустой вектор, нулевые значения.  
- Большие векторы (1000 элементов).  

Результаты последовательной и MPI-версий совпадают для всех тестов. В MPI-режиме корневой процесс получает сумму векторов всех процессов, остальные — нулевой вектор.  

### 7.2 Производительность

Методы измерений:
- Каждый тест запускается 5 раз
- Берется среднее время выполнения (ΣTime / 5)
- Speedup = Time_seq / Time_mpi
- Efficiency = Speedup / Count * 100%

| Mode        | Count | Time, s        | Speedup  | Efficiency |
|-------------|-------|----------------|----------|------------|
| seq         | 1     | 0.00749415078  | 1.00     | N/A        |
| mpi         | 2     | 0.05543197644  | 0.135    | 6.75%      |
| mpi         | 4     | 0.11167683796  | 0.088    | 2.20%      |
| mpi         | 6     | 0.12228601788  | 0.111    | 1.85%      |

Анализ результатов:
1. Кастомная реализация работает в 7-11 раз медленнее последовательной версии
2. С увеличением числа процессов эффективность падает до менее 2%
3. Основная причина - высокие накладные расходы MPI операций при малом объеме вычислений

## 8. Заключение
В ходе работы была реализована кастомная операция Reduce с использованием алгоритма бинарного дерева. Несмотря на корректность работы, реализация показала низкую производительность из-за:
1. Высоких накладных расходов на коммуникации
2. Малого объема вычислений относительно коммуникационных затрат
3. Отсутствия оптимизаций, присутствующих в стандартной MPI_Reduce
Работа демонстрирует важность баланса между объемом вычислений и коммуникациями при проектировании параллельных алгоритмов.

## 9. Источники
1. Документация по курсу «Параллельное программирование» // URL: https://learning-process.github.io/parallel_programming_course/ru/index.html
2. Репозиторий курса «Параллельное программирование» // URL: https://github.com/learning-process/ppc-2025-processes-engineers
3. Сысоев А. В., Лекции по курсу «Параллельное программирование для кластерных систем».

## Приложение
П.1
```cpp
bool CopyVector(const InType &input, OutType &output) {
  auto &input_vec = std::get<std::vector<T>>(input);
  auto &output_vec = std::get<std::vector<T>>(output);

  if (output_vec.size() != input_vec.size()) {
    output_vec.resize(input_vec.size());
  }

  if (!input_vec.empty()) {
    std::memcpy(output_vec.data(), input_vec.data(), input_vec.size() * sizeof(T));
  }
  return true;
}

bool KotelnikovaAFromAllToOneSEQ::RunImpl() {
  try {
    auto input = GetInput();
    auto &output = GetOutput();

    if (std::holds_alternative<std::vector<int>>(input)) {
      return CopyVector<int>(input, output);
    }
    if (std::holds_alternative<std::vector<float>>(input)) {
      return CopyVector<float>(input, output);
    }
    if (std::holds_alternative<std::vector<double>>(input)) {
      return CopyVector<double>(input, output);
    }

    return false;
  } catch (...) {
    return false;
  }
}
```

П.2
```cpp
namespace {
    void PerformOperationImpl(void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype) {
    if (datatype == MPI_INT) {
        auto *in = static_cast<int *>(inbuf);
        auto *inout = static_cast<int *>(inoutbuf);
        for (int i = 0; i < count; i++) {
        inout[i] += in[i];
        }
    } else if (datatype == MPI_FLOAT) {
        auto *in = static_cast<float *>(inbuf);
        auto *inout = static_cast<float *>(inoutbuf);
        for (int i = 0; i < count; i++) {
        inout[i] += in[i];
        }
    } else if (datatype == MPI_DOUBLE) {
        auto *in = static_cast<double *>(inbuf);
        auto *inout = static_cast<double *>(inoutbuf);
        for (int i = 0; i < count; i++) {
        inout[i] += in[i];
        }
    } else {
        throw std::runtime_error("Unsupported datatype");
    }
    }
}  // namespace

namespace kotelnikova_a_from_all_to_one {

bool KotelnikovaAFromAllToOneMPI::RunImpl() {
  try {
    auto input = GetInput();
    int rank = 0;
    int root = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (std::holds_alternative<std::vector<int>>(input)) {
      return ProcessVector<int>(input, rank, root, MPI_INT);
    }

    if (std::holds_alternative<std::vector<float>>(input)) {
      return ProcessVector<float>(input, rank, root, MPI_FLOAT);
    }

    if (std::holds_alternative<std::vector<double>>(input)) {
      return ProcessVector<double>(input, rank, root, MPI_DOUBLE);
    }

    return false;
  } catch (...) {
    return false;
  }
}

template <typename T>
bool KotelnikovaAFromAllToOneMPI::ProcessVector(const InType &input, int rank, int root, MPI_Datatype mpi_type) {
  auto &original_data = std::get<std::vector<T>>(input);

  if (original_data.empty()) {
    return true;
  }

  if (rank == root) {
    auto &output_variant = GetOutput();
    auto &result_data = std::get<std::vector<T>>(output_variant);
    std::ranges::copy(original_data, result_data.begin());
    CustomReduce(result_data.data(), result_data.data(), static_cast<int>(original_data.size()), mpi_type, MPI_SUM,
                 MPI_COMM_WORLD, root);
  } else {
    std::vector<T> send_buffer = original_data;
    CustomReduce(send_buffer.data(), nullptr, static_cast<int>(original_data.size()), mpi_type, MPI_SUM, MPI_COMM_WORLD,
                 root);
  }
  return true;
}

void KotelnikovaAFromAllToOneMPI::CustomReduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                                               MPI_Op op, MPI_Comm comm, int root) {
  if (count == 0) {
    MPI_Barrier(comm);
    return;
  }

  int size = 0;
  MPI_Comm_size(comm, &size);

  int rank = 0;
  MPI_Comm_rank(comm, &rank);

  if (rank == root) {
    TreeReduce(sendbuf, recvbuf, count, datatype, op, comm, root);
  } else {
    int type_size = 0;
    MPI_Type_size(datatype, &type_size);
    size_t total_bytes = static_cast<size_t>(count) * static_cast<size_t>(type_size);
    std::vector<unsigned char> temp_buf(total_bytes);
    TreeReduce(sendbuf, temp_buf.data(), count, datatype, op, comm, root);
  }
}

void KotelnikovaAFromAllToOneMPI::TreeReduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                                             MPI_Comm comm, int root) {
  int size = 0;
  MPI_Comm_size(comm, &size);

  int rank = 0;
  MPI_Comm_rank(comm, &rank);

  if (count == 0) {
    MPI_Barrier(comm);
    return;
  }

  if (op != MPI_SUM) {
    return;
  }

  int type_size = 0;
  MPI_Type_size(datatype, &type_size);
  size_t total_bytes = static_cast<size_t>(count) * static_cast<size_t>(type_size);

  std::vector<unsigned char> local_buf(total_bytes);
  std::memcpy(local_buf.data(), sendbuf, total_bytes);

  int depth = 0;
  while ((1 << depth) < size) {
    depth++;
  }

  for (int level = 0; level < depth; level++) {
    int mask = 1 << level;
    int partner = rank ^ mask;

    if (partner >= size) {
      continue;
    }

    if ((rank & mask) == 0) {
      if (partner < size) {
        std::vector<unsigned char> recv_buf(total_bytes);
        MPI_Recv(recv_buf.data(), count, datatype, partner, 0, comm, MPI_STATUS_IGNORE);
        PerformOperation(recv_buf.data(), local_buf.data(), count, datatype);
      }
    } else {
      MPI_Send(local_buf.data(), count, datatype, partner, 0, comm);
      break;
    }
  }

  if (rank == root && recvbuf != nullptr) {
    std::memcpy(recvbuf, local_buf.data(), total_bytes);
  }
}

void KotelnikovaAFromAllToOneMPI::PerformOperation(void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype) {
  PerformOperationImpl(inbuf, inoutbuf, count, datatype);
}

template bool KotelnikovaAFromAllToOneMPI::ProcessVector<int>(const InType &input, int rank, int root,
                                                              MPI_Datatype mpi_type);
template bool KotelnikovaAFromAllToOneMPI::ProcessVector<float>(const InType &input, int rank, int root,
                                                                MPI_Datatype mpi_type);
template bool KotelnikovaAFromAllToOneMPI::ProcessVector<double>(const InType &input, int rank, int root,
                                                                 MPI_Datatype mpi_type);
}
```