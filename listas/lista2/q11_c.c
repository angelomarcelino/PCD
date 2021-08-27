#include <math.h>
#include <mpi.h> /* For MPI functions, etc */
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* For strlen */
#include <time.h>

int main() {
	//int size = 8;
	int arr[] = {0, 1, 2, 3, 4, 5, 6, 7};
	int comm_sz, my_rank;

	MPI_Init(NULL, NULL);
	MPI_Comm comm = MPI_COMM_WORLD;
	MPI_Comm_size(comm, &comm_sz);
	MPI_Comm_rank(comm, &my_rank);


	int *local_arr = (int *)malloc(comm_sz * sizeof(int));

	int lim = round(log10(comm_sz) / log10(2));
	for (int d = 1; d < lim; d++) {
		//printf("proc %d iter %d desloc %d lim %d\n", my_rank, d, 1<<d, lim);
		if ((my_rank + 1) % (1 << d) == 0) {
			arr[my_rank] += arr[my_rank - (1 << (d - 1))];
			//printf("proc %d pos %d iter %d desloc %d value %d\n", my_rank, my_rank - (1 << (d - 1)), d, 1 << d, arr[my_rank]);
		}
		for (int j = 0; j < comm_sz; j++) {
			local_arr[my_rank] = arr[my_rank];
			MPI_Bcast(local_arr, comm_sz, MPI_INT, j, comm);
		}
	}

	for (int d = lim; d < 0; d--) {
		//printf("proc %d iter %d desloc %d lim %d\n", my_rank, d, 1<<d, lim);
		if ((my_rank + 1) % (1 << d) == 0) {
			arr[my_rank] += arr[my_rank - (1 << (d - 1))];
			//printf("proc %d pos %d iter %d desloc %d value %d\n", my_rank, my_rank - (1 << (d - 1)), d, 1 << d, arr[my_rank]);
		}
		for (int j = 0; j < comm_sz; j++) {
			local_arr[my_rank] = arr[my_rank];
			MPI_Bcast(local_arr, comm_sz, MPI_INT, j, comm);
		}
	}

	if (my_rank == 0) {
		printf("Prefix Sum\n\t");
		for (int i = 0; i < comm_sz; i++) {
			printf("%d  ", local_arr[i]);
		}
		printf("\n");
	}

	free(local_arr);

	MPI_Finalize();
	return 0;
}