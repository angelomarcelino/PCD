#include <math.h>
#include <mpi.h> /* For MPI functions, etc */
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* For strlen */
#include <time.h>

double randfrom(double min, double max) {
	double range = (max - min);
	double div = RAND_MAX / range;
	return min + (rand() / div);
}

void Read_vector(double local_arr[], int local_n, int n,
				 char vec_name[], int my_rank,
				 MPI_Comm comm) {
	double *arr = NULL;

	if (my_rank == 0) {
		arr = malloc(n * sizeof(double));
		printf("Generating the vector %s\n", vec_name);
		for (int i = 0; i < n; i++) {
			//arr[i] = randfrom(0.0, 1.0);
			arr[i] = i + 1.0;
			//printf("%.2f ", arr[i]);
		}
		//printf("\n");
		MPI_Scatter(arr, local_n, MPI_DOUBLE,
					local_arr, local_n, MPI_DOUBLE, 0, comm);
		free(arr);
	} else {
		MPI_Scatter(arr, local_n, MPI_DOUBLE, local_arr, local_n,
					MPI_DOUBLE, 0, comm);
	}
}

void Read_int(char msg[], int *num, int my_rank,
			  int comm_sz, MPI_Comm comm, int number) {
	if (my_rank == 0) {
		//printf("%s", msg);
		//scanf("%d", num);
		*num = number;
	}
	MPI_Bcast(num, 1, MPI_INT, 0, comm);
}

void Read_double(char msg[], double *num, int my_rank,
				 int comm_sz, MPI_Comm comm, double number) {
	if (my_rank == 0) {
		//printf("%s", msg);
		//scanf("%lf", num);
		*num = number;
	}
	MPI_Bcast(num, 1, MPI_DOUBLE, 0, comm);
}

void ScalarProd(double local_arr[], double scalar, int local_n) {
	for (int i = 0; i < local_n; i++)
		local_arr[i] *= scalar;
}

void DotProduct(double local_x[], double local_y[], double *result,
				int local_n) {
	for (int i = 0; i < local_n; i++) {
		*result += local_x[i] * local_y[i];
	}
}

int main(int argc, char **argv) {
	srand(time(NULL));

	int comm_sz, my_rank, vec_size;
	int local_n;
	double *local_x, *local_y;
	double scalar, local_sum, result;

	MPI_Init(NULL, NULL);
	MPI_Comm comm = MPI_COMM_WORLD;
	MPI_Comm_size(comm, &comm_sz);
	MPI_Comm_rank(comm, &my_rank);

	Read_int("Entre o tamanho dos vetores:\n", &vec_size, my_rank,
			 comm_sz, comm, atoi(argv[1]));
	Read_double("Entre o escalar:\n", &scalar, my_rank,
				comm_sz, comm, atof(argv[2]));

	if (vec_size % comm_sz != 0) return -1;

	local_n = vec_size / comm_sz;
	local_x = malloc(local_n * sizeof(double));
	local_y = malloc(local_n * sizeof(double));

	Read_vector(local_x, local_n, vec_size, "X", my_rank, comm);
	Read_vector(local_y, local_n, vec_size, "Y", my_rank, comm);

	ScalarProd(local_x, scalar, local_n);
	DotProduct(local_x, local_y, &local_sum, local_n);

	MPI_Reduce(&local_sum, &result, 1, MPI_DOUBLE, MPI_SUM, 0, comm);

	MPI_Barrier(comm);
	printf("P%d --> S%.2f --> LN%d\n", my_rank, local_sum, local_n);
	/*
    for (int i = 0; i < local_n; i++) {
		printf("P%d RX %.2f\n", my_rank, local_x[i]);
		printf("P%d RY %.2f\n", my_rank, local_y[i]);
	}
	printf("\n");
    */

	if (my_rank == 0) printf("\nMy result = %.2f\n", result);

	MPI_Finalize();
	return 0;
}
/* main */
