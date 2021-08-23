#include <mpi.h> /* For MPI functions, etc */
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* For strlen */

double f(double x) {
	return -(x * x) + 16;
}

int main(int argc, char **argv) {
	int comm_sz, my_rank, n, local_n;
	double a, b, h, local_a, local_b;
	double local_intg, total_intg = 0.0;

	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

	if (argc == 4) {
		a = atof(argv[1]);
		b = atof(argv[2]);
		n = atoi(argv[3]);
		if (my_rank == 0) printf("%f %f %d\n", a, b, n);
	} else {
		if (my_rank == 0) printf("Wrong arguments\n");
		return -1;
	}
	MPI_Barrier(MPI_COMM_WORLD);

	total_intg = 0.0;

	h = (b - a) / n;
	
	local_n = n / comm_sz;
	int r = n % comm_sz;

	if (my_rank < r) {
		local_n += 1;
		local_a = a + my_rank * local_n * h;
		printf("%f ", local_a);
	} else {
		local_a = a + (r * h) + (my_rank * local_n * h);
		printf("%f ", local_a);
	}
	MPI_Barrier(MPI_COMM_WORLD);

	local_b = local_a + local_n * h;
	printf("%f ", local_b);
	printf("%d\n", local_n);
	MPI_Barrier(MPI_COMM_WORLD);
	// Local integral
	local_intg = (f(local_a) + f(local_b)) / 2.0;

	for (int i = 0; i < local_n; i++) {
		double x_i = local_a + i * h;
		local_intg += f(x_i);
	}
	local_intg = h * local_intg;

    /* // Messaging
	if (my_rank != 0) {
		MPI_Send(&local_intg, 1, MPI_DOUBLE,
				 0, 0, MPI_COMM_WORLD);
	} else {
		total_intg = local_intg;
		for (int p = 1; p < comm_sz; p++) {
			MPI_Recv(&local_intg, 1, MPI_DOUBLE, p, 0,
					 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			total_intg += local_intg;
		}
	} // */

	MPI_Reduce(&local_intg, &total_intg, 1, MPI_DOUBLE, MPI_SUM, 0,
			   MPI_COMM_WORLD);

	if(my_rank == 0) printf("%f\n", total_intg);

	MPI_Finalize();
	return 0;
}
/* main */
