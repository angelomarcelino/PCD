#include <math.h>
#include <mpi.h> /* For MPI functions, etc */
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* For strlen */
#include <time.h>

int main() {
	int size = 5;
	int arr[] = {1, 2, 3, 4, 5};
	int pSum[size];
	int comm_sz, my_rank;

	MPI_Init(NULL, NULL);
	MPI_Comm comm = MPI_COMM_WORLD;
	MPI_Comm_size(comm, &comm_sz);
	MPI_Comm_rank(comm, &my_rank);

	if (comm_sz != size) return -1;

	int my_psum = 0;
	for (int i = 0; i <= my_rank; i++) {
		my_psum += arr[i];
	}

    if(my_rank != 0){
		MPI_Send(&my_psum, 1, MPI_INT, 0, 0, comm);
	} else {
		pSum[0] = my_psum;
        for (int i = 1; i < comm_sz; i++)
        {
			MPI_Recv(&pSum[i], 1, MPI_INT, i, 0, comm, MPI_STATUS_IGNORE);
			printf("recieved %d from %d \n", pSum[i], i);
		}
	}

    if (my_rank == 0)
    {
		printf("Prefix Sum\n\t");
		for (int i = 0; i < size; i++) {
			printf("%d  ", pSum[i]);
		}
		printf("\n");
	}

	MPI_Finalize();
	return 0;
}
