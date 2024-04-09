#include <stdio.h>
#include <stdlib.h>
#include <omp.h>


// Инициализации матрицы и вектора
void init(double *matrix, double *vector, int n, int m, int threads) 
{
    for (int i = 0; i < n; i++) 
    {
        vector[i] = 1.0;
    }
    #pragma omp parallel num_threads(threads)
    {
        int tid = omp_get_thread_num();
        int size = m / threads; // Определение размера кластера матрицы-вектора для каждого потока
        if (m % threads != 0)
        {
            if (m % threads > threads / 2) size++; // Оптимизация алгоритма по опросу остатка
        }
        int start = size * n * tid; // Переменная начала кластера матрицы
        int end = (tid == threads - 1) ? n * m : (tid + 1) * size * n ; // Переменная конца кластера матрицы, последний
        // кластер берет включая оставшиеся нечетные вектора подматрицы

        for (int i = start; i < end; i += m) 
        {
           for (int j = 0; j < n; j++)
           {
                matrix[j+i] = tid;
           }
        }
    }
}

// Функция для перемножения матрицы-вектора на вектор
void mult_matrix_vector(double *matrix, double *vector, double *result, int n, int m, int threads) 
{
    #pragma omp parallel num_threads(threads)
    {
        int tid = omp_get_thread_num();
        int size = m / threads; // Определение размера кластера матрицы-вектора для каждого потока
        if (m % threads != 0)
        {
            if (m % threads > threads / 2) size++; // Оптимизация алгоритма по опросу остатка
        }
        int start = size * n * tid; // Переменная начала кластера матрицы
        int end = (tid == threads - 1) ? n * m : (tid + 1) * size * n ; // Переменная конца кластера матрицы, последний
        // кластер берет включая оставшиеся нечетные вектора подматрицы

        for (int i = start; i < end; i+=m) 
        {
            int row = i / m;
            result[row] = 0.0;
            for (int j = 0, k = 0; k < m; j++, k++) 
            {
                result[row] += matrix[j+i] * vector[k];
            }
        }
    }
}

int main(int argc, char *argv[]) 
{
    if (argc<4)
    {
        printf("Too few parametors!\n");
        exit(1);
    }
    else if (argc>4)
    {
        printf("Too many parametors!\n");
        exit(1);
    }

    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    int threads = atoi(argv[3]);

    double *matrix = (double*)malloc(n*m*sizeof(double));
    double *vector = (double*)malloc(n * sizeof(double));
    double *result = (double*)malloc(m * sizeof(double));;

    init(matrix, vector, n, m, threads);


    double start_time = omp_get_wtime();
    mult_matrix_vector(matrix, vector, result, n, m, threads);
    double end_time = omp_get_wtime();

    // for (int i = 0; i < m; i++) 
    // {
    //     for (int j = 0; j < n; j++) 
    //     {
    //         printf("%0.2f ", matrix[j + i * n]);
    //     }
    //    printf("\n");
    // }
    // printf("\n");
    // for (int i = 0; i < m; i++) 
    // {
    //    printf("%0.2f ", vector[i]);
    // }
    // printf("\n\n");

    // for (int i = 0; i < m; i++) 
    // {
    //    printf("%0.2f ", result[i]);
    // }
    // printf("\n\n");
    printf("Time taken: %f seconds\n", end_time - start_time);

    return 0;
}