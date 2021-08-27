#include <math.h>
#include <mpi.h> /* For MPI functions, etc */
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* For strlen */
#include <time.h>

int main() {
	int size = 8;
	int arr[] = {0, 1, 2, 3, 4, 5, 6, 7};
	int comm_sz, my_rank;

	MPI_Init(NULL, NULL);
	MPI_Comm comm = MPI_COMM_WORLD;
	MPI_Comm_size(comm, &comm_sz);
	MPI_Comm_rank(comm, &my_rank);

	if (comm_sz != size)
		return -1;

	int my_num, my_sum;
	MPI_Scatter(arr, 1, MPI_INT, &my_num, 1, MPI_INT, 0, comm);

	if (my_rank == 0) {
		printf("Prefix Sum\n\t");
		printf("%d  ", my_num);
		MPI_Send(&my_num, 1, MPI_INT, my_rank + 1, 0, comm);
		for (int i = 1; i < comm_sz; i++) {
			MPI_Recv(&my_num, 1, MPI_INT, i, 0, comm, MPI_STATUS_IGNORE);
			printf("%d  ", my_num);
		}
		printf("\n");
	} else {
		MPI_Recv(&my_sum, 1, MPI_INT, my_rank - 1, 0, comm, MPI_STATUS_IGNORE);
		my_sum += my_num;
		MPI_Send(&my_sum, 1, MPI_INT, 0, 0, comm);
		if (my_rank < comm_sz - 1) {
			MPI_Send(&my_sum, 1, MPI_INT, my_rank + 1, 0, comm);
		}
	}

	MPI_Finalize();
	return 0;
}
