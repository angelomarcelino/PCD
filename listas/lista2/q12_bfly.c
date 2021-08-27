#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
	int comm_sz;
	int my_rank;

	MPI_Init(NULL, NULL);
	MPI_Comm comm = MPI_COMM_WORLD;
	MPI_Comm_size(comm, &comm_sz);
	MPI_Comm_rank(comm, &my_rank);

	if ((1 << (int)round(log2(comm_sz))) != comm_sz) {
		if (my_rank == 0)
			printf("Number of processors (%d) is not a power of 2!\n", comm_sz);
		MPI_Finalize();
		return 0;
	}

	int *local_arr = NULL;
	int local_num;

	if (my_rank == 0) {
		local_arr = (int *)malloc(comm_sz * sizeof(int));
		//printf("Arr\n");
		for (int i = 0; i < comm_sz; i++) {
			local_arr[i] = i + 1;
			//printf("%d ", local_arr[i]);
		}
		//printf("\n");
		MPI_Scatter(local_arr, 1, MPI_INT, &local_num, 1, MPI_INT, 0, comm);
	} else {
		MPI_Scatter(local_arr, 1, MPI_INT, &local_num, 1, MPI_INT, 0, comm);
	}

	double local_st, local_fn, local_pass, elapsed;
	MPI_Barrier(comm);
	local_st = MPI_Wtime();

	int sum = local_num;
	int partner;
	int recv_num;

	for (int i = 0; i < (int)round(log2(comm_sz)); i++) {
		int group = (1 << (i + 1));
		if ((my_rank % group) < (group / 2)) {
			partner = my_rank + (group / 2);
		} else {
			partner = my_rank - (group / 2);
		}
		//printf("proc %d partner %d group %d\n", my_rank, partner, group);
		//MPI_Barrier(comm);
		MPI_Sendrecv(&sum, 1, MPI_INT, partner, 0, &recv_num, 1, MPI_INT, partner, 0, comm, MPI_STATUS_IGNORE);
		//if (my_rank == 0) printf("proc %d sent %d recieved %d\n", my_rank, sum, recv_num);
		sum += recv_num;
	}

	MPI_Gather(&sum, 1, MPI_INT, local_arr, 1, MPI_INT, 0, comm);
	
    local_fn = MPI_Wtime();
	local_pass = local_fn - local_st;
	MPI_Reduce(&local_pass, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
	
    if (my_rank == 0) {
		//for (int i = 0; i < comm_sz; i++) {
		//	printf("proc %d sum %d\n", i, local_arr[i]);
		//}
		printf("Tempo de execução: %e s\n", elapsed);
	}

    

	if (local_arr)
		free(local_arr);

	MPI_Finalize();

	return 0;
}