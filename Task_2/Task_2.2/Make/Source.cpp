#include <iostream>
#include <math.h>
#include <omp.h>

double func(double x) {
    // Пример функции для интегрирования
    return exp(-x * x);
}

/*
 Данная фунция принимает указатель на функцию func которая будет интегрироваться, 
 и границы интегрирования a и b. n определяет количество прямоугольников, 
 используемых для численного интегрирования.
*/
double integrate_omp(double (*func)(double), double a, double b, int n)
{
    // h представляет шаг интегрирования
    double h = (b - a) / n;
    double sum = 0.0;

    #pragma omp parallel    
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();

        // количество прямоугольников на каждый отдельный поток.
        int items_per_thread = n / nthreads;
        // индекс первого прямоугольника, обрабатываемого текущим потоком.
        int lb = threadid * items_per_thread;
        // индекс последнего прямоугольника, обрабатываемого текущим потоком.
        int ub = (threadid == nthreads - 1) ? (n - 1) : (lb + items_per_thread - 1);
        double sumloc = 0.0;

        for (int i = lb; i <= ub; i++)
            sumloc += func(a + h * (i + 0.5));

        //  атомарное выполнение операции сложения для переменной sum, чтобы избежать гонок данных.
        #pragma omp atomic
            sum += sumloc;
    }
    // корректировка результата, усреднение площади сумм прямоугольников - интеграла
    sum *= h;
    return sum;
}

int main() {
    const int steps = 40000000;
    const double a = 0.0;
    const double b = 10.0;

    std::cout << "Колличество потоков: " << omp_get_max_threads() << std::endl << std::endl;

    omp_set_num_threads(1);
    // Вычисляем время выполнения последовательной версии
    double start_time = omp_get_wtime();
    double integral_seq = integrate_omp(func, a, b, steps);
    double end_time = omp_get_wtime();
    double seq_time = end_time - start_time;

    std::cout << "Результат интеграла (последовательная программа): " << integral_seq << std::endl;
    std::cout << "Время: " << seq_time << " секунд" << std::endl << std::endl;

    // Вычисляем время выполнения параллельной версии для разного числа потоков
    for (int num_threads : {2, 4, 7, 8, 16, 20, 40}) {
        omp_set_num_threads(num_threads);
        start_time = omp_get_wtime();
        double integral_omp = integrate_omp(func, a, b, steps);
        end_time = omp_get_wtime();
        double omp_time = end_time - start_time;

        std::cout << "Результат интеграла (параллельная программа): (" << num_threads << " потоков): " << integral_omp << std::endl;
        std::cout << "Время (" << num_threads << " потоков): " << omp_time << " секунд" << std::endl;
        std::cout << "Ускорение (" << num_threads << " потоков): " << seq_time / omp_time << std::endl << std::endl;
    }

    return 0;
}