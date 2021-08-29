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

void Read_vector(double local_arr[], int n,
				 char vec_name[], int my_rank,
				 MPI_Comm comm, MPI_Datatype dtype) {
	double *arr = NULL;

	if (my_rank == 0) {
		arr = malloc(n * sizeof(double));
		printf("Generating the vector %s\n", vec_name);
		for (int i = 0; i < n; i++) {
			arr[i] = randfrom(0.0, 1.0);
			printf("%.2f ", arr[i]);
		}
		printf("\n");

		MPI_Scatter(arr, 1, dtype,
					local_arr, 1, dtype, 0, comm);
		free(arr);
	} else {
		MPI_Scatter(arr, 1, dtype, local_arr, 1,
					dtype, 0, comm);
	}
}

void Print_vector(double local_arr[], int n, int my_rank, MPI_Comm comm,
				  MPI_Datatype dtype) {
	double *arr = malloc(n * sizeof(double));

	MPI_Gather(local_arr, 1, dtype, arr, 1, dtype, 0, comm);

	if (my_rank == 0) {
		for (int i = 0; i < n; i++)
			printf("%.2f ", arr[i]);
		printf("\n");
	}

	free(arr);
}
int main(int argc, char **argv) {
	srand(time(NULL));

	int comm_sz, my_rank, vec_size;
	int local_n;
	double *local_x;
	double scalar;
	double *x = NULL;

	MPI_Init(NULL, NULL);
	MPI_Comm comm = MPI_COMM_WORLD;
	MPI_Comm_size(comm, &comm_sz);
	MPI_Comm_rank(comm, &my_rank);

	if (my_rank == 0) {
		vec_size = atoi(argv[1]);
		printf("Tamanho dos vetores: %d\n", vec_size);
		scalar = atof(argv[2]);
		printf("Escalar: %.2f\n", scalar);
	}

	MPI_Bcast(&vec_size, 1, MPI_INT, 0, comm);
	MPI_Bcast(&scalar, 1, MPI_DOUBLE, 0, comm);

	if (vec_size % comm_sz != 0) return -1;

	local_n = vec_size / comm_sz;
	local_x = malloc(local_n * sizeof(double));

	MPI_Datatype MPI_DOUBLE_ALT;
	MPI_Type_contiguous(local_n, MPI_DOUBLE, &MPI_DOUBLE_ALT);
	MPI_Type_commit(&MPI_DOUBLE_ALT);

	Read_vector(local_x, vec_size, "X", my_rank, comm, MPI_DOUBLE_ALT);

	for (int i = 0; i < local_n; i++) {
		local_x[i] *= scalar;
	}

	if (my_rank == 0)
		printf("X scaled by %.2f:\n", scalar);

	Print_vector(local_x, vec_size, my_rank, comm, MPI_DOUBLE_ALT);

	// --------------------------------------------

	free(x);
	free(local_x);
	MPI_Type_free(&MPI_DOUBLE_ALT);

	MPI_Finalize();

	return 0;
}
/* main */
