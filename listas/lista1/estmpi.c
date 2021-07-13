#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	int thread_count = strtol(argv[1], NULL, 10);
	int iter_count = strtol(argv[2], NULL, 10);

	double pi_estm = 0.0;
	double sum = 0.0;
	double factor = 1.0;
#pragma omp parallel for num_threads(thread_count) \
	reduction(+: sum) private(factor)
	for (int i = 0; i < iter_count; i++){
		factor = (i % 2 == 0) ? 1.0 : -1.0;
		sum += factor / (2 * i + 1);
	}

	pi_estm = 4.0 * sum;
	printf("%.20f \n", pi_estm);

	return 0;
}