#include <mpi.h> /* For MPI functions, etc */
#include <stdio.h>
#include <string.h> /* For strlen */

const int MAX_STRING = 100;

int main(void) {
	char greeting[MAX_STRING];
	int comm_sz;
	/* Number of processes */
	int my_rank;
	/* My process rank */

	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

	if (my_rank != 0) {
		sprintf(greeting, "Greetings from process %d of %d!",
				my_rank, comm_sz);
		MPI_Send(greeting, strlen(greeting) + 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
	} else {
		printf("Greetings from process %d of %d!\n", my_rank,
			   comm_sz);
		for (int q = 1; q < comm_sz; q++) {
			int ret = MPI_Recv(greeting, MAX_STRING, MPI_CHAR, MPI_ANY_SOURCE, 0, 
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			printf("%d ---- %d\n", q, ret);
			printf("%s\n", greeting);
		}
	}

	MPI_Finalize();
	return 0;
}
/* main */
