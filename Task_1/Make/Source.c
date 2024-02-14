#define MAX 10000000
#define M_PI 3.14159265358979323846
#include<stdio.h>
#include<math.h>
#include<time.h>

#ifdef FLOAT
    typedef float arr_type;
    #define SIN_FUNC sinf
    
#else
    typedef double arr_type;
    #define SIN_FUNC sin
#endif

arr_type arr[MAX];

int main()
{   
    clock_t start, end;
    arr_type buff = 0;

    start = clock();

    for (long long int i=0;i<MAX;i++)
    {
       arr[i]=SIN_FUNC((2*M_PI*i)/MAX);
    }
  
    for (long long int i=0;i<MAX;i++)
    {
        buff += arr[i];
    }

    end = clock();
    double result = (double)(end-start);
    // перевод в секунды
    result = result / CLOCKS_PER_SEC;
    printf("Result = %0.14f\n", buff);
    printf("Time = %0f Seconds\n", result);
    return 0;
}
