#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	int thread_count = strtol(argv[1], NULL, 10);
	int iter_count = strtol(argv[2], NULL, 10);


#pragma omp parallel num_threads(thread_count)
{
	double time_s, time_f;

	time_s = omp_get_wtime();

	double sum = 0.0;
    for (int i = 0; i < iter_count; i++)
    {
        #pragma omp atomic
		sum += sin(i);
	}

	time_f = omp_get_wtime();

    #pragma omp critical
	printf("Thread %d finished in %.5f seconds.\n", omp_get_thread_num(), time_f - time_s);
}
	return 0;
}