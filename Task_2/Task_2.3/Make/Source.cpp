#include <iostream>
#include <cstdlib> // для работы с функциями стандартной библиотеки C, такими как atoi.
#include <cmath> // для математических операций, таких как fabs - абсолютное значние числа вещественного или модуль
#include <omp.h>

#define N 120 // Размер пластинки (NxN)
#define EPSILON 0.00001 // Точность для завершения итераций (порог ошибки)

// Эта функция инициализирует сетку 
// (двумерный массив, представленный в виде одномерного) с размером N x N. 
double* init_grid(int size_x, int size_y)
{
    double* grid = new double[size_x*size_y];
    for (int i = 0; i < N; i++) 
    {
        for (int j = 0; j < N; j++) 
        {
            grid[j+N*i] = (i == 0) ? 20.0 : 0.0;
        }
    }
    grid[N*N-2] = -20;
    return grid;
}

//  функция проводит одну итерацию вычислений для обновления температуры на сетке и
//  вычисления максимальной ошибки
double sequence_temp(double* grid, double* grid_swap, int size_x, int size_y)
{
    double error = 0.0;
    
    // что такое редукция в данном контексте -  это набор манипуляций с переменной в параллельной логике работы, а именно:
    // Для каждого потока OpenMP создаёт свою локальную копию переменной error. Эти копии будут использоваться независимо в каждом потоке.
    // В каждом потоке error будет обновляться независимо, согласно вычислениям, которые происходят внутри цикла.
    // После завершения всех итераций параллельного цикла, OpenMP автоматически объединит все локальные копии переменной error, 
    // выбрав максимальное значение из них и сохранив его в глобальную переменную error, доступную в основном потоке.
    #pragma omp parallel for reduction(max:error)  // без reduction будет гонка данных за error
    //(можно использовать локальную переменную и critical, но зачем)) 
    // MAX:error - операция которую проводим между вычисленными локальными максимумами в потоках
    for (int i = 1; i < N - 1; i++) 
    {
        for (int j = 1; j < N - 1; j++) 
        {
            grid[j+N*i] = (grid_swap[(j-1)+N*i]+grid_swap[(j+1)+N*i]+grid_swap[j+N*(i-1)]+grid_swap[j+N*(i+1)]) / 4.0;
            error = fabs(grid_swap[j+N*i]-grid[j+N*i])>error ? fabs(grid_swap[j+N*i]-grid[j+N*i]) : error;
        }
    }
    return error;
    // Использование директивы reduction в OpenMP снижает количество блокировок за счёт локального вычисления 
    // и откладывает синхронизацию до конца параллельного блока
}

int main(int argc, char* argv[]) 
{

    int threads;
    if (argc < 2) threads = 1;
    else threads = atoi(argv[1]);

    std::cout << "Максимальное колличество потоков: " << omp_get_max_threads() << std::endl << std::endl;

    // Устанавливается количество потоков для OpenMP
    omp_set_num_threads(threads);
    double start_time = omp_get_wtime(); // захват начального времени работы программы

    double* grid = init_grid(N, N);
    double* grid_swap = init_grid(N, N);
    double error = 1.0;

    #ifdef FIRST
    while (error > EPSILON)
    {
        error = sequence_temp(grid, grid_swap,N,N);
        // std::cout << "ERROR -> " << error << std::endl;
        std::swap(grid, grid_swap);
    }
    #else 
    #ifdef SECOND
    // В данном блоке кода компиллятор использует многопоточность только для вычислений блоков с циклом for
    // собственно, они там и помечены через #pragma omp collapse(2) nowait (чисто для наглядности, потому что компиллятор сам это применяет)
    // поэтому остальное выполняется последовательно, а значит и не надо использовать никакую синхронизацию кода и гонок данных нет
    #pragma omp parallel
    {
        while (error > EPSILON)
        {
            // #pragma omp critical
            // {
                error = 0;
            // }
            double loc_error = 0.0;

            #pragma omp collapse(2) nowait
            for (int i = 1; i < N - 1; i++) 
            {
                for (int j = 1; j < N - 1; j++) 
                {
                    grid[j+N*i] = (grid_swap[(j-1)+N*i]+grid_swap[(j+1)+N*i]+grid_swap[j+N*(i-1)]+grid_swap[j+N*(i+1)]) / 4.0;
                }
            }

            #pragma omp collapse(2) nowait // добавлено чисто для визуализации как работает (то есть можно и без нее)
            for (int i = 1; i < N - 1; i++) 
            {
                for (int j = 1; j < N - 1; j++) 
                {
                    loc_error = fabs(grid_swap[j+N*i]-grid[j+N*i]) > loc_error ? fabs(grid_swap[j+N*i]-grid[j+N*i]) : loc_error;
                }
            }
            // #pragma omp critical
            // {
                // if (loc_error > error) 
                // {
                    error = loc_error;
                    // std::cout << "ERROR -> " << error << std::endl;
                // }
            // }
            // std::cout << "ERROR -> " << error << std::endl;
            std::swap(grid, grid_swap);
        }
    }
    #else
    std::cout << "Не выбран нужный вариант компилляции!!!\n";
    #endif
    #endif
    

    double end_time = omp_get_wtime();
    double time = end_time - start_time;

    std::cout << "Колличество потоков: " << threads << std::endl << std::endl;
    std::cout << "Время: " << time << " секунд" << std::endl << std::endl;

    delete[] grid;
    delete[] grid_swap;

    return 0;
}
