#include <cstdlib>
#include <chrono>
#include <malloc.h>
#include <iostream>
#include <boost/program_options.hpp>
#include <cuda_runtime.h>
#include <cublas_v2.h>


// // лямбда функция максимума для редукции
#define max(x, y) ((x) > (y) ? (x) : (y) )

namespace po = boost::program_options;
namespace tim = std::chrono;

bool boost_parser(int argc, char *argv[], int &size, double &accur, int &iter_max)
{
	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "Show help message")
		("accuracy,a", po::value<double>(&accur)->default_value(0.01), "Set accuracy")
		("grid_size,g", po::value<int>(&size)->default_value(100), "Set grid size")
		("num_iterations,n", po::value<int>(&iter_max)->default_value(1000), "Set number of iterations");

	po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }
    } catch (const po::error &ex) {
        std::cerr << ex.what() << std::endl;
        return 1;
    }
	return 0;
}

int main(int argc, char *argv[])
{
	//объявляем "size of matrix", "accurancy" и "iterations_max"

	int size = 0;
	double accur = 0.0;
	int iter_max = 0;
	// функция обработчика аргументов терминала
	bool err = boost_parser(argc, argv, size, accur, iter_max);
    if (err == 1)
	{
		std::cerr << "Program have error in arg_parser!!!\n";
		exit(1);
	}

	//смещение массива на 2 индекса:
    // 1 - в связи с тем что происходит обрез матрицы по "правому" и "нижнему" краю(<20 а не =20)
    // 2 - в свзи с тем что при пробеге в циклах по массиву по "правому" и "нижнему" краю будет браться
    // мнимый ноль на границах чтобы не выйти за предел матрицы (см схему ниже под кодом)
	int bias_1 = 2;
	// создание двумерного массива с учетом добавления обреза
	// создание двух матриц нужно для того чтобы можно было вычитать ячейки проработанного массива
	// и находить ошибку и быстро переобновлять матрицу

    auto start_time = tim::high_resolution_clock::now();

    double* main_arr= (double*)malloc(((size + bias_1)*(size+bias_1)) * sizeof(double));
    double* main_arr2 = (double*)malloc(((size + bias_1)*(size+bias_1)) * sizeof(double));

	// объявление переменной отсчета итерации и градиента = шага нарастания от одной вершины до другой  
	int iter = 0;
	double gradient = 10.0 / size;
	// объявление переменной ошибки 
	double error = 1.0;
    // двойной указатель на массив как контейнер обмена данными между 2 матрицами
    double *ptr;
    // Структурированные данные — это данные, которые организованы в определенные и фиксированные форматы, такие как массивы, структуры и т.д.
    // Неструктурированные данные — это данные, которые не имеют фиксированной или предсказуемой структуры.
	/* Так как у нас НЕструктурированая модель данных (динамический массив) мы выделяем память на ГПУ под размеры соответствующие
		нашим массивам с помощью create и ТОЛЬКО копируем данные (copyin) на ГПУ ускоритель и заполняем этот массив на ГПУ памяти
        enter - используем для того чтобы не заключать в скобки {} код работы на устройсте и автоперенос на хост после завершения
        периода жизни данных на карточке
	*/
	#pragma acc enter data create(main_arr[0:(size + bias_1)*(size+bias_1)],main_arr2[0:(size + bias_1)*(size+bias_1)]) copyin(size, gradient, bias_1)
    // Заполнение данных на устройстве

    // указываем явно на девайсе с чем именно мы будем сейчас работать
    #pragma acc data present(main_arr, main_arr2, size, gradient, bias_1)
    // возмем по 4 блока потоков(типа в каждом цикле по 4 строки операции, пусть на каждую по группе будет)
    #pragma acc parallel num_gangs(4)
    {
        // Первый цикл для main_arr
        #pragma acc loop independent gang vector() 
        for (int i = 0; i < size + bias_1; i++)
        {
            main_arr[i*(size+2) + 0] = 10 + gradient*i;
            main_arr[i] = 10 + gradient*i;
            main_arr[(size+1)*(size+2) + i] = 20 + gradient*i;
            main_arr[i*(size+2)+size+1] = 20 + gradient*i;
        }

        // Второй цикл для main_arr2
        #pragma acc loop independent gang vector()
        for (int i = 0; i < size + bias_1; i++)
        {
            main_arr2[i*(size+2) + 0] = main_arr[i*(size+2) + 0];
            main_arr2[i] = main_arr[i];
            main_arr2[(size+1)*(size+2) + i] = main_arr[(size+1)*(size+2) + i];
            main_arr2[i*(size+2)+size+1] = main_arr[i*(size+2)+size+1];
        }
    }

    // cublasHandle_t в CUDA является структурой, представляющей собой обработчик (handle) 
    // для работы с библиотекой cuBLAS (CUDA Basic Linear Algebra Subroutines). Он используется для 
    // инициализации и управления ресурсами, необходимыми для выполнения операций линейной алгебры на GPU.
    // Этот обработчик обеспечивает доступ к функциям библиотеки cuBLAS, таким как матричные операции 
    // (умножение, сложение, вычитание), скалярные операции и другие операции линейной алгебры, которые
    // могут выполняться параллельно на графическом процессоре.

    cublasHandle_t handle;
    
    cublasCreate(&handle);
    

    int index; // индекс максимального элемента
    double max; // значение по индексу выше
    double alpha = (-1.0); // коэффициент alpha, используемый в операции cublasDaxpy

    while ((error > accur) && (iter<iter_max))
    {
        iter++;


        #pragma acc data present(main_arr, main_arr2)
        #pragma acc parallel async(2)
        {
            #pragma acc loop gang vector()
            for (int j = 1; j < size + 1; j++)
            {
                #pragma acc loop gang vector()
                for (int i = 1; i < size + 1; i++)
                {
                    main_arr2[i*(size+2)+j] = 0.25 * (main_arr[(i+1)*(size+2)+j] + main_arr[(i-1)*(size+2)+j] + main_arr[i*(size+2)+j-1] + main_arr[i*(size+2)+j+1]);
                }
            }
        }

        if ((iter % 150 == 0) || (iter == 1))
        {
            #pragma acc wait(2)
            
            // Директива, указывающая, что массивы main_arr 
            // и main_arr2 используются на устройстве, но их данные могут 
            // быть доступны на хосте для операций cuBLAS.....Крч просто быстрая копия из карты обратно на процессор
            // код ниже в скобках исполняется на CPU
            #pragma acc host_data use_device(main_arr, main_arr2)
            {
                // Данная функция выполняет операцию умножения и сложения 
                // main_arr = main_arr + alpha * main_arr2 где единицы это шаги инкрементации по указателям
                cublasDaxpy(handle, ((size+2)*(size+2)), &alpha, main_arr2, 1, main_arr, 1);
                // ну тут по названию понятно, поиск индекса максимально значения (оговорочка - по модулю!)
                cublasIdamax(handle, ((size+2)*(size+2)), main_arr, 1, &index);
            }
            // Обновляет значения элемента main_arr на хосте с использованием данных с устройства, чтобы вычислить max.
            #pragma acc update host(main_arr[index-1:1])
            max = main_arr[index-1];
            // Находит максимальное по модулю значение в массиве main_arr.
            if (max < 0)  max = max *(-1);

            #pragma acc host_data use_device(main_arr, main_arr2)
            {
                // Копирует данные main_arr2 в main_arr на устройстве 
                cublasDcopy(handle, ((size+2)*(size+2)), main_arr2, 1, main_arr, 1);
            }

            error = max;
            printf("Iteration - %d ; Error = %0.14lf\n", iter, error);
        }


        ptr = main_arr;
        main_arr = main_arr2;
        main_arr2 = ptr;

    }

    cublasDestroy(handle);

	auto end_time = tim::high_resolution_clock::now();
    auto duration = tim::duration_cast<tim::milliseconds>(end_time - start_time);
    printf("Iteration - %d ; Error = %lf\n", iter, error);
    std::cout <<" Time: " << duration.count() << " ms" << std::endl;

	return 0;

}

/*  схема строения тензора: 8 x 8

g(i) = 10 + gradient*i
f(i) = 20 + gradient*i
i = index

i = {0 .. 7} - 8 элементов
i = i + 2 = {0 .. 9} элементов - 7-ой элемент не равен значению вершины тензора
поэтому мы делаем +1 смещение для получения полного тензора без обреза по краям снизу и справа (index = 8);
далее делаем еще одно смещение +1 где мы получаем два мнимых края тензора c рандомными значеним 
чтобы при применении уравнения теплопроводности на точках (x, 8) и (8, x) где x -> {0 .. 8}
мы не заходили за пределы тензора или за область допустимой памяти:

main_arr2[i][j] = 0.25 * (main_arr[i+1][j] + main_arr[i-1][j] + main_arr[i][j-1] + main_arr[i][j+1]);


_i_|_0 _|_1 _|_2 _|_3 _|_4 _|_5 _|_6 _|_7 _|_8 _|_9 _
_0_|_10_|_1g_|_2g_|_3g_|_4g_|_5g_|_6g_|_7g_|_20_|_??_
_1_|_1g_|_x _|_x _|_x _|_x _|_x _|_x _|_x _|_1f_|_??_
_2_|_2g_|_x _|_x _|_x _|_x _|_x _|_x _|_x _|_2f_|_??_
_3_|_3g_|_x _|_x _|_x _|_x _|_x _|_x _|_x _|_3f_|_??_
_4_|_4g_|_x _|_x _|_x _|_x _|_x _|_x _|_x _|_4f_|_??_
_5_|_5g_|_x _|_x _|_x _|_x _|_x _|_x _|_x _|_5f_|_??_
_6_|_6g_|_x _|_x _|_x _|_x _|_x _|_x _|_x _|_6f_|_??_
_7_|_7g_|_x _|_x _|_x _|_x _|_x _|_x _|_x _|_7f_|_??_
_8_|_20_|_1f_|_2f_|_3f_|_4f_|_5f_|_6f_|_7f_|_30_|_??_
_9_|_??_|_??_|_??_|_??_|_??_|_??_|_??_|_??_|_??_|_??_

*/