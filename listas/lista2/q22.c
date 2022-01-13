#include <mpi.h> /* For MPI functions, etc */
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* For strlen */

double f(double x) {
	return x * x;
}

int main(int argc, char **argv) {
	int comm_sz, my_rank, local_n;
	int n;
	double a, b, h, local_a, local_b;
	double local_intg, total_intg = 0.0;

	MPI_Init(NULL, NULL);
	MPI_Comm comm = MPI_COMM_WORLD;
	MPI_Comm_size(comm, &comm_sz);
	MPI_Comm_rank(comm, &my_rank);

	if (argc == 4) {
		a = atof(argv[1]);
		b = atof(argv[2]);
		n = atoi(argv[3]);
		//if (my_rank == 0) printf("NumProc = %d\nWith n = %d, a = %.2f, b = %.2f\n\t", comm_sz, n, a, b);
	} else {
		if (my_rank == 0) printf("Wrong number of arguments\n");
		return -1;
	}
	MPI_Barrier(comm);

	total_intg = 0.0;
	h = (b - a) / n;
	local_n = n / comm_sz;
	int r = n % comm_sz;

	if (my_rank < r) {
		local_n += 1;
		local_a = a + my_rank * local_n * h;
	} else {
		local_a = a + (r * h) + (my_rank * local_n * h);
	}

	local_b = local_a + local_n * h;

	// Timing starts
    double local_start, local_finish, local_elapsed;
	double max, min, mean;
	MPI_Barrier(comm);
	local_start = MPI_Wtime();

	// Local integral
	local_intg = (f(local_a) + f(local_b)) / 2.0;

	for (int i = 1; i < local_n; i++) {
		double x_i = local_a + i * h;
		local_intg += f(x_i);
	}
	local_intg = h * local_intg;


	//Timing Ends
	local_finish = MPI_Wtime();
	local_elapsed = local_finish - local_start;
	MPI_Reduce(&local_elapsed, &max, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
	MPI_Reduce(&local_elapsed, &min, 1, MPI_DOUBLE, MPI_MIN, 0, comm);
	MPI_Reduce(&local_elapsed, &mean, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
	mean = mean / comm_sz;
	double *times = malloc(comm_sz * sizeof(double));
	double median;
	MPI_Gather(&local_elapsed, 1, MPI_DOUBLE, times, 1, MPI_DOUBLE, 0, comm);

	double temp = 0.0;
	for (int i = 0; i < comm_sz - 1; i++) {
		for (int j = i + 1; j < comm_sz; j++)
			if(times[i]>times[j]){
				temp = times[i];
				times[i] = times[j];
				times[j] = temp;
			}
	}

	if(comm_sz%2 == 0){
		int posA = (comm_sz / 2);
		int posB = (comm_sz / 2) + 1;
		median = (times[posA] + times[posB]) / 2.0;
	} else if (comm_sz > 1){
		median = times[(comm_sz - 1) / 2];
	} else {
		median = local_elapsed;
	}

	MPI_Reduce(&local_intg, &total_intg, 1, MPI_DOUBLE, MPI_SUM, 0,
			   comm);

	double mult = 1000000.0;
	if (my_rank == 0) {
		for (int i = 0; i < comm_sz; i++)
        {
			//printf("%.2f ", times[i] * mult);
		}

		printf("%.2f %.2f %.2f %.2f\n", max * mult, min * mult, mean * mult, median * mult);
	}

	MPI_Finalize();
	return 0;
}
/* main */
