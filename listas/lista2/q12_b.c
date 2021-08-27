#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
	int comm_sz;
	int my_rank;

	int *local_arr = NULL;
	int local_num;

	MPI_Init(NULL, NULL);
	MPI_Comm comm = MPI_COMM_WORLD;
	MPI_Comm_size(comm, &comm_sz);
	MPI_Comm_rank(comm, &my_rank);

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

	int sum = 0;
	int recv_from;
	int send_to;

	if (my_rank == 0) {
		recv_from = comm_sz - 1;
		send_to = 1;
	} else if (my_rank == (comm_sz - 1)) {
		recv_from = (comm_sz - 2);
		send_to = 0;
	} else {
		recv_from = (my_rank - 1);
		send_to = (my_rank + 1);
	}

	if (my_rank == 0) {
		sum += local_num;
		MPI_Send(&sum, 1, MPI_INT, send_to, 0, comm);
	} else if (my_rank < comm_sz - 1) {
		MPI_Recv(&sum, 1, MPI_INT, recv_from, 0, comm, MPI_STATUS_IGNORE);
		sum += local_num;
		MPI_Send(&sum, 1, MPI_INT, send_to, 0, comm);
	} else {
		MPI_Recv(&sum, 1, MPI_INT, recv_from, 0, comm, MPI_STATUS_IGNORE);
		sum += local_num;
	}

	MPI_Gather(&sum, 1, MPI_INT, local_arr, 1, MPI_INT, 0, comm);

	local_fn = MPI_Wtime();
	local_pass = local_fn - local_st;
	MPI_Reduce(&local_pass, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, comm);

	if (my_rank == 0) {
		for (int i = 0; i < comm_sz; i++) {
			printf("proc %d sum %d\n", i, local_arr[i]);
		}
		printf("Tempo de execução: %e s\n", elapsed);
	}

	if (local_arr)
		free(local_arr);

	MPI_Finalize();

	return 0;
}