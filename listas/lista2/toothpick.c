#include <mpi.h> /* For MPI functions, etc */
#include <stdio.h>
#include <string.h> /* For strlen */

int main(void) {
	int comm_sz;
	/* Number of processes */
	int my_rank;
	/* My process rank */

	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

	printf("Proc %d of %d > Does anyone have a toothpick?\n", my_rank, comm_sz);

	MPI_Finalize();
	return 0;
}
/* main */
