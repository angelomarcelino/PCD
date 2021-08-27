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


int main(int argc, char **argv) {
	srand(time(NULL));

	int comm_sz, my_rank, vec_size;
	int local_n;
	double *local_x, *local_y;
	double scalar, local_sum, result;
	double *x = NULL, *y = NULL;

	MPI_Init(NULL, NULL);
	MPI_Comm comm = MPI_COMM_WORLD;
	MPI_Comm_size(comm, &comm_sz);
	MPI_Comm_rank(comm, &my_rank);

	if (my_rank == 0) {
		vec_size = atoi(argv[1]);
		printf("Tamanho dos vetores: %d\n", vec_size);
		scalar = atof(argv[2]);
		printf("Escalar: %.2f\n", scalar);

		x = malloc(vec_size * sizeof(double));
		y = malloc(vec_size * sizeof(double));

		printf("Generating the vector X\n");
		for (int i = 0; i < vec_size; i++) {
			x[i] = randfrom(0.0, 1.0);
			//x[i] = i + 1;
			printf("%.2f ", x[i]);
		}
		printf("\n");
		printf("Generating the vector Y\n");
		for (int i = 0; i < vec_size; i++) {
			y[i] = randfrom(0.0, 1.0);
			//y[i] = i + 1;
			printf("%.2f ", y[i]);
		}
		printf("\n");
	}

	MPI_Bcast(&vec_size, 1, MPI_INT, 0, comm);
	MPI_Bcast(&scalar, 1, MPI_DOUBLE, 0, comm);

	int *counts_send, *disp;
	counts_send = (int *)malloc(comm_sz * sizeof(int));
	disp = (int *)calloc(comm_sz, sizeof(int));

	for (int i = 0; i < comm_sz; i++) {
		int r = vec_size % comm_sz;
		int q = vec_size / comm_sz;
		counts_send[i] = i < r ? q + 1 : q;
	}

	for (int i = 1; i < comm_sz; i++) {
		disp[i] = disp[i - 1] + counts_send[i - 1];
	}

	/*if (my_rank == 0){
		printf("\n");
		for (int i = 0; i < comm_sz; i++)
			printf("%d ", counts_send[i]);
	    printf("\n\n");
		for (int i = 0; i < comm_sz; i++)
			printf("%d ", disp[i]);
		printf("\n\n");
	}
    
    
    for (int i = 0; i < counts_send[my_rank]; i++) {
		printf("proc %d item %d is %.2f displ %d\n", my_rank, i, local_arr[i], disp[my_rank]);
	}
    */

	local_x = (double *)malloc(counts_send[my_rank] * sizeof(double));
	local_y = (double *)malloc(counts_send[my_rank] * sizeof(double));

	MPI_Scatterv(x, counts_send, disp, MPI_DOUBLE,
				 local_x, counts_send[my_rank], MPI_DOUBLE, 0, comm);
	MPI_Scatterv(y, counts_send, disp, MPI_DOUBLE,
				 local_y, counts_send[my_rank], MPI_DOUBLE, 0, comm);

	local_n = counts_send[my_rank];

	for (int i = 0; i < local_n; i++)
		local_x[i] *= scalar;
	for (int i = 0; i < local_n; i++) {
		local_sum += local_x[i] * local_y[i];
	}

	MPI_Gatherv(local_x, counts_send[my_rank], MPI_DOUBLE,
				x, counts_send, disp, MPI_DOUBLE, 0, comm);

	MPI_Reduce(&local_sum, &result, 1, MPI_DOUBLE, MPI_SUM, 0, comm);

	if (my_rank == 0){
		printf("\nX scaled\n");
		for (int i = 0; i < vec_size; i++)
			printf("%.2f ", x[i]);
		printf("\n\nResultado de %.2fX . Y = %.3f\n", scalar, result);
	}

	// --------------------------------------------

	free(x);
    free(y);
    free(local_x);
	free(local_y);

	MPI_Finalize();

	return 0;
}
/* main */
