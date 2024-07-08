#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>

#define ROWS 5000
#define COLS 5000

using namespace std;
using namespace std::chrono;

// Функция для параллельной инициализации массивов
void parallel_initialize(double *matrix, double* vec, int start, int end) 
{
    for (int i = start; i < end; ++i) 
    {
        for (int j = 0; j < COLS; ++j) 
        {
            
            matrix[j + i*COLS] = i;//static_cast<double>(rand()) / RAND_MAX; - нельзя это использовать,так как это однопоточная
            // реализация генератора рандомных чисел, что грубо говоря превращает код в последовательную программу.
            // аналог с использованием мьютексов-синхронизацией(однопоточное узкое горлышко бутылки)
        }
        vec[i] = 2;//static_cast<double>(rand()) / RAND_MAX;
    }
}

// Функция для параллельного умножения матрицы на вектор
void parallel_multiply(double* matrix, double* vec, double* result, int start, int end) 
{
    for (int i = start; i < end; ++i) 
    {
        result[i] = 0;
        for (int j = 0; j < COLS; ++j) 
        {
            result[i] += matrix[j + i*COLS] * vec[j];
        }
    }
}

int main() {

    double *matrix = new double[ROWS*COLS];
    double *vec = new double[COLS];
    double *result = new double[ROWS];

    // Количество потоков для теста
    int thread_counts[] = {1, 2, 4, 7, 8, 16, 20, 40};

    for (int thread_count : thread_counts) 
    {
        thread threads[thread_count];
        auto start_time = high_resolution_clock::now();

        // Инициализация матрицы и вектора
        int chunk_size = ROWS / thread_count;
        for (int i = 0; i < thread_count; ++i) 
        {
            int start = i * chunk_size;
            int end = (i == thread_count - 1) ? ROWS : start + chunk_size;

            threads[i] = thread(parallel_initialize, matrix, vec, start, end);
        }
        for (int i = 0; i < thread_count; ++i)
        {
            threads[i].join();
        }

        // Умножение матрицы на вектор
        for (int i = 0; i < thread_count; ++i) 
        {
            int start = i * chunk_size;
            int end = (i == thread_count - 1) ? ROWS : start + chunk_size;

            threads[i] = thread(parallel_multiply, matrix, vec, result, start, end);
        }
        for (int i = 0; i < thread_count; ++i) 
        {
            threads[i].join();
        }

        auto end_time = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end_time - start_time);

        cout << "Threads: " << thread_count << " Time: " << duration.count() << " ms" << endl;
    }

    delete[] matrix;
    delete[] vec;
    delete[] result;

    return 0;
}

