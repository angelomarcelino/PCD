#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	int thread_count = strtol(argv[1], NULL, 10);
	int iter_count = strtol(argv[2], NULL, 10);

#pragma omp parallel for num_threads(thread_count)
	for (int i = 0; i < iter_count; i++){
		int my_rank = omp_get_thread_num();

        #pragma omp critical
        printf("Thread %d: Iteration %d\n", my_rank, i);
	}

		return 0;
}