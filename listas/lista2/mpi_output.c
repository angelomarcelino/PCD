/* File:     mpi_output.c
 *
 * Purpose:  A program in which multiple MPI processes try to print 
 *           a message.
 *
 * Compile:  mpicc -g -Wall -o mpi_output mpi_output.c
 * Usage:    mpiexec -n<number of processes> ./mpi_output
 *
 * Input:    None
 * Output:   A message from each process
 *
 * IPP:      Section 3.3.1  (pp. 97 and ff.)
 */
#include <mpi.h>
#include <stdio.h>
#include <string.h>

const int MAX_STRING = 100;

int main(void) {
	int my_rank, comm_sz;
	char hi[MAX_STRING];

	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

	if (my_rank != 0) {
		sprintf(hi, "Proc %d of %d > Does anyone have a toothpick?\n",
				my_rank, comm_sz);
		MPI_Send(hi, strlen(hi) + 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
	} else {
		printf("Proc %d of %d > Does anyone have a toothpick?\n",
			   my_rank, comm_sz);
        for (int i = 1; i < comm_sz; i++)
        {
			MPI_Recv(hi, MAX_STRING, MPI_CHAR, i, 0, MPI_COMM_WORLD,
					 MPI_STATUS_IGNORE);
			printf("%s", hi);
		}
	}

	

	MPI_Finalize();
	return 0;
} /* main */
