#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <condition_variable>
#include <mutex>


#define ROWS 5000
#define COLS 5000

using namespace std;
using namespace std::chrono;

std::condition_variable cv;
std::mutex cv_m;
bool ready = false;

// Функция для параллельной инициализации массивов
void initialize_and_multiply(double* matrix, double* vec, double* result, int start, int end) 
{
    // Инициализация матрицы и вектора
    for (int i = start; i < end; ++i) 
    {
        for (int j = 0; j < COLS; ++j) 
        {
            matrix[j + i * COLS] = i;
        }
        vec[i] = 2;
    }

    // Сигнал о завершении инициализации и ожидание начала умножения
    {
        // это RAII-класс, который захватывает мьютекс при создании объекта и освобождает его при выходе из области видимости.
        // cv_m — это мьютекс, который защищает разделяемую переменную ready
        std::lock_guard<std::mutex> lk(cv_m);
        ready = true;
        // После захвата мьютекса поток устанавливает флаг ready в true, что сигнализирует о завершении инициализации.
    }

    // будит все потоки, которые ждут на условной переменной cv.
    // Это сигнал для всех потоков, что можно продолжать выполнение (начинать умножение).
    cv.notify_all();

    // Ожидание сигнала для начала умножения
    {
        // Здесь поток захватывает мьютекс cv_m, используя объект lk. Далее ниже
        std::unique_lock<std::mutex> lk(cv_m);
        // cv.wait освобождает мьютекс и переводит поток в режим ожидания до тех пор, пока условие не станет истинным.
        // Когда поток просыпается, он повторно захватывает мьютекс .
        // Лямбда-выражение [] { return ready; } проверяет, стал ли флаг ready истинным. Если ready все еще false, поток продолжает ждать.
        // Это гарантирует, что поток не начнет умножение до тех пор, пока инициализация не завершена.
        cv.wait(lk, [] { return ready; });
    }

    // Умножение матрицы на вектор
    for (int i = start; i < end; ++i) 
    {
        result[i] = 0;
        for (int j = 0; j < COLS; ++j) 
        {
            result[i] += matrix[j + i * COLS] * vec[j];
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

            threads[i] = thread(initialize_and_multiply, matrix, vec, result, start, end);
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

/*
Мьютекс (Mutex)

Мьютекс (mutex, сокращение от "mutual exclusion" — взаимное исключение) — это примитив синхронизации, 
который используется для защиты разделяемого ресурса от одновременного доступа нескольких потоков. 
Только один поток может владеть мьютексом в каждый момент времени. Если другой поток попытается захватить 
заблокированный мьютекс, он будет ждать, пока мьютекс не станет доступен.

std::lock_guard:

    Используется для простого и эффективного захвата и освобождения мьютекса.
    Идеально подходит для случаев, когда требуется захватить мьютекс на короткое время без сложных манипуляций.

std::unique_lock:

    Предоставляет более гибкое управление мьютексом.
    Обязателен при использовании с условными переменными, так как позволяет временно освобождать мьютекс и 
    повторно захватывать его после выхода из состояния ожидания.
*/