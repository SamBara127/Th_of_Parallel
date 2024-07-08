#include <cuda_runtime.h>
#include <chrono>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h> 
#include <malloc.h>
#include <cuda.h>
#include <iostream>
#include <cub/cub.cuh>
#include <boost/program_options.hpp>
#define MAX_THREADS 32

namespace po = boost::program_options;
namespace tim = std::chrono;

bool boost_parser(int argc, char *argv[], int &size, double &accur, int &iter_max, int &show_map)
{
	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "Show help message")
		("accuracy,a", po::value<double>(&accur)->default_value(0.01), "Set accuracy")
		("grid_size,g", po::value<int>(&size)->default_value(100), "Set grid size")
		("num_iterations,n", po::value<int>(&iter_max)->default_value(1000), "Set number of iterations")
        ("mode,m", po::value<int>(&show_map)->default_value(0), "Enable or disable map_grid");

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

__global__ void init(double *arr, int size, int threads, int blocks, double gradient)
{
    long long int i = (blockIdx.x*(threads*threads)) + (blockIdx.y * (threads*threads*blocks)) + (threadIdx.x + threads * threadIdx.y);
	arr[i] = 15;
} 

__global__ void debug(double *arr, int size, int threads, int blocks, double gradient)
{
    long long int i = (blockIdx.x*(threads*threads)) + (blockIdx.y * (threads*threads*blocks)) + (threadIdx.x + threads * threadIdx.y);
	if ((i>=size+2) && (i<(size+2)*(size+1)) && ((i % (size+2)) != 0) && (((i+1) % (size+2)) != 0)) 
	{
		arr[i] = 77;
	}
} 

__global__ void kernel(double *arr, int size, int threads, int blocks, double gradient)
{
    long long int i = (blockIdx.x*(threads*threads)) + (blockIdx.y * (threads*threads*blocks)) + (threadIdx.x + threads * threadIdx.y);
	if (i<(size+2)*(size+1)-1) 
	{
		if ((i>=0) && (i<=size)) arr[i] =  10 + gradient*i;
		if ((i % (size+2)) == 0) arr[i] =  10 + (gradient*i)/(size+2);
		if ((i>=(size+2)*(size)) && (i<=(size+2)*(size+1))) arr[i] =  20 + (gradient*(i - (size+2)*(size)));
		if (((i+2) % (size+2)) == 0) arr[i] =  20 + (gradient*i)/(size+2);
	}
} 

__global__ void temp_comp(double *arr, double* arr2, int size, int threads, int blocks)
{
    long long int i = (blockIdx.x*(threads*threads)) + (blockIdx.y * (threads*threads*blocks)) + (threadIdx.x + threads * threadIdx.y);
	if ((i>=size+2) && (i<(size+2)*(size+1)) && ((i % (size+2)) != 0) && (((i+1) % (size+2)) != 0)) 
	{
		arr2[i] = 0.25 * (arr[i-(size+2)] + arr[i+(size+2)] + arr[i+1] + arr[i-1]);
	}
} 

__global__ void difference(double *arr, double* arr2, double* diff_arr, int threads, int blocks)
{
    long long int i = (blockIdx.x*(threads*threads)) + (blockIdx.y * (threads*threads*blocks)) + (threadIdx.x + threads * threadIdx.y);
	diff_arr[i] = arr2[i] - arr[i];
} 


// int qw = size+4;
	// main_arr[qw] = 44;

	// main_arr[qw-(size+bias_1)] = 44;
	// main_arr[qw+(size+bias_1)] = 44;
	// main_arr[qw+1] = 44;
	// main_arr[qw-1] = 44;

int main(int argc, char *argv[])
{
	int size = 0;
	double accur = 0.0;
	int iter_max = 0;
    int show_map = 0;
	// функция обработчика аргументов терминала
	bool err = boost_parser(argc, argv, size, accur, iter_max, show_map);
    if (err == 1)
	{
		std::cerr << "Program have error in arg_parser!!!\n";
		exit(1);
	}

	if (show_map == 0) printf("--------SHOW MAP COMPUTING = 'OFF'--------\n"); else printf("--------SHOW MAP COMPUTING = 'ON'--------\n");

	printf ("PART 1: ALLOCATE OUR ARRAYS IN VRAM AND RAM\n");

    int int_blocks = 0;
    int mod_blocks = 0;
	
	int bias_1 = 2;

	size_t gl_size = (size + bias_1)*(size + bias_1)*sizeof(double);
	
    auto start_time = tim::high_resolution_clock::now();

	double* main_arr = (double*)malloc(gl_size);
    double* main_dev;
    double* main_arr2 = (double*)malloc(gl_size);
    double* main2_dev;

	double* reduce_arr = (double*)malloc(gl_size);
    double* reduce_arr_d;

	double* out_max_data;


	int iter = 0;
	double gradient = 10.0 / size;
	double error = 1.0;
    double *ptr_d;

	cudaError_t cuda_er;

	cuda_er = cudaMalloc(&ptr_d, gl_size);
	assert(cuda_er == 0);
	printf("------ALLOCATED MEMORY GPU_POINTER SUCCESS!!!\n");
	cuda_er = cudaMalloc(&main_dev, gl_size);
	assert(cuda_er == 0);
	printf("------ALLOCATED MEMORY 1 SUCCESS!!!\n");
	cuda_er = cudaMalloc(&main2_dev, gl_size);
	assert(cuda_er == 0);
	printf("------ALLOCATED MEMORY 2 SUCCESS!!!\n");	
	cuda_er = cudaMalloc(&reduce_arr_d, gl_size);
	assert(cuda_er == 0);
	printf("------ALLOCATED MEMORY REDUCE SUCCESS!!!\n");

	cuda_er = cudaMalloc(&out_max_data, sizeof(double));
	assert(cuda_er == 0);
	printf("------ALLOCATED MEMORY REDUCE_BUFFER SUCCESS!!!\n");

	printf ("PART 2: FILL OUR ARRAYS IN VRAM AND RAM\n");

	int threads = MAX_THREADS;
    int_blocks = (size + bias_1);
    mod_blocks = int_blocks % threads;
    int_blocks = (mod_blocks == 0) ? int_blocks / threads : (int_blocks / threads)+1;
    int blocks = int_blocks;

    dim3 THREADS(threads,threads);
    dim3 BLOCKS(blocks,blocks);

	printf("------GRID_MAP-COMPUTE STRUCTURE -- BLOCKS = %d , THREADS = %d \n", blocks, threads);

	printf("------RUN KERNEL (kernel)\n");

	//init<<<BLOCKS, THREADS>>>(main_dev, size, threads, blocks, gradient);
	kernel<<<BLOCKS, THREADS>>>(main_dev, size, threads, blocks, gradient);
	//debug<<<BLOCKS, THREADS>>>(main_dev, size, threads, blocks, gradient);

	cuda_er = cudaMemcpy(main_arr, main_dev, gl_size, cudaMemcpyDeviceToHost);
    assert(cuda_er == 0);
    printf("------COPIOUT VRAM (arr № 1) -> RAM (arr № 1) SUCCESSFULLY!!!\n");

	cuda_er = cudaMemcpy(main2_dev, main_dev, gl_size, cudaMemcpyDeviceToDevice);
    assert(cuda_er == 0);
    printf("------COPIOUT VRAM (arr № 1) -> VRAM (arr № 2) SUCCESSFULLY!!!\n");

	cuda_er = cudaMemcpy(main_arr2, main2_dev, gl_size, cudaMemcpyDeviceToHost);
    assert(cuda_er == 0);
    printf("------COPIOUT VRAM (arr № 2) -> RAM (arr № 2) SUCCESSFULLY!!!\n");
	
	printf ("PART 3: CREATE CUB_REDUCTIN FEATURE TO FAST-FIND MAX DATA\n");

	void *buffer_cub_device = NULL;
	size_t size_buffer_cub_dev = 0;

	cub::DeviceReduce::Max(buffer_cub_device, size_buffer_cub_dev, reduce_arr_d, out_max_data, (size + bias_1)*(size + bias_1));
	cuda_er = cudaMalloc(&buffer_cub_device, size_buffer_cub_dev);
	assert(cuda_er == 0);
	printf("------ALLOCATED MEMORY CUB_REDUCE SUCCESS!!!\n");

	printf ("PART 4: RUN ITERATION COMPUTING...\n");

	cudaStream_t stream_1;
    cudaStreamCreate(&stream_1);

	while ((error > accur) && (iter<iter_max))
    {
        iter++;
		temp_comp<<<BLOCKS, THREADS, 0, stream_1>>>(main_dev, main2_dev, size, threads, blocks);

		if ((iter % 150 == 0) || (iter == 1))
		{
			difference<<<BLOCKS, THREADS>>>(main_dev, main2_dev, reduce_arr_d, threads, blocks);
			cub::DeviceReduce::Max(buffer_cub_device, size_buffer_cub_dev, reduce_arr_d, out_max_data, (size + bias_1)*(size + bias_1));
			cudaMemcpy(&error, out_max_data, sizeof(double), cudaMemcpyDeviceToHost);

			printf("Iteration - %d ; Error = %0.14lf\n", iter, error);
		}

		ptr_d = main_dev;
		main_dev = main2_dev;
		main2_dev = ptr_d;
	} 
	printf("Iteration - %d ; Error = %0.14lf\n", iter, error);
	
	auto end_time = tim::high_resolution_clock::now();
    auto duration = tim::duration_cast<tim::milliseconds>(end_time - start_time);

	if (show_map == 1)
	{
		printf ("PART 5: SHOW FINALLY MAP COMPUTING:\n");

		cuda_er = cudaMemcpy(main_arr, main_dev, gl_size, cudaMemcpyDeviceToHost);
		assert(cuda_er == 0);
		printf("------OK\n");

		int k=0;
		for (int i=0;i<(size + bias_1)*(size + bias_1);i++)
		{
			if (k==(size + bias_1))
			{
				printf("\n");
				k = 0;
			}
			k = k +1;
			printf("%d ", (int)main_arr[i]);
		}
		printf("\n");
		printf ("PART 6: FREE OUR ARRAYS AND DATA FROM VRAM:\n");
	} else 
	{
		printf ("PART 5: FREE OUR ARRAYS AND DATA FROM VRAM:\n");
	}

	cuda_er = cudaFree(main_dev);
    assert(cuda_er == 0);
    printf("------FREE MEMORY (arr № 1) SUCCESS!!!\n");
	cuda_er = cudaFree(main2_dev);
    assert(cuda_er == 0);
    printf("------FREE MEMORY (arr № 2) SUCCESS!!!\n");
	cuda_er = cudaFree(reduce_arr_d);
    assert(cuda_er == 0);
    printf("------FREE MEMORY (reduce arr) SUCCESS!!!\n");
	// cuda_er = cudaFree(ptr_d);
    // assert(cuda_er == 0);
    // printf("------FREE MEMORY (pointer) SUCCESS!!!\n");
	cuda_er = cudaFree(out_max_data);
    assert(cuda_er == 0);
    printf("------FREE MEMORY (out_max_buffer) SUCCESS!!!\n");
	cuda_er = cudaFree(buffer_cub_device);
    assert(cuda_er == 0);
    printf("------FREE MEMORY (cub_data) SUCCESS!!!\n");

	printf("PROGRAMM HAS ENDED SUCCESSFULLY!!!\n");
	std::cout << "Result: Iteration -" << iter << "; Error = ;" << error << " Time: " << duration.count() << " ms" << std::endl;

	return 0;

}