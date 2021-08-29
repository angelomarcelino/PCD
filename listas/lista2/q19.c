#include <math.h>
#include <mpi.h> /* For MPI functions, etc */
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* For strlen */
#include <time.h>

double randfrom(double min, double max) {
	double range = (max - min);
	double div = RAND_MAX / range;
	return min + (rand() / div);
}

int main(int argc, char **argv) {
	srand(time(NULL));

	int comm_sz, my_rank, vec_size;

	MPI_Init(NULL, NULL);
	MPI_Comm comm = MPI_COMM_WORLD;
	MPI_Comm_size(comm, &comm_sz);
	MPI_Comm_rank(comm, &my_rank);
	if (comm_sz != 2) {
		printf("Essa aplicação é feita para rodar com 2 processos.\n");
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
	}

	// Ler o tamanho do vetor e cria-lo
	double *mat = NULL;
	if (my_rank == 0) {
		vec_size = atoi(argv[1]);
		printf("Tamanho do vetor: %d\n", vec_size);
		printf("Gerando matriz %dx%d:\n", vec_size, vec_size);
		mat = malloc(vec_size * vec_size * sizeof(double));
		for (int i = 0; i < vec_size; i++) {
			for (int j = 0; j < vec_size; j++) {
				//mat[i * vec_size + j] = i * vec_size + j;
				mat[i * vec_size + j] = randfrom(0, 100);
                printf("%.2f\t", mat[i * vec_size + j]);
			}
			printf("\n");
		}
	}
	MPI_Bcast(&vec_size, 1, MPI_INT, 0, comm);

	// Criar o DataType e enviar o triangulo pra o proc 1
	MPI_Datatype MPI_UpperTriangle;
	int lengths[vec_size];
	int displs[vec_size];

	for (int i = 0; i < vec_size; i++) {
		lengths[i] = vec_size - i;
		displs[i] = (i * vec_size) + i;  // pula uma linha e vai pra diagonal
	}

	MPI_Type_indexed(vec_size, lengths, displs, MPI_DOUBLE, &MPI_UpperTriangle);
	MPI_Type_commit(&MPI_UpperTriangle);

	if (my_rank == 0) {
		MPI_Send(mat, 1, MPI_UpperTriangle, 1, 0, comm);
	} else {
		int size = vec_size * (vec_size + 1) / 2;
		
        double *triangle = (double *)calloc(size, sizeof(double));
        MPI_Recv(triangle, size, MPI_DOUBLE, 0, 0, comm, MPI_STATUS_IGNORE);

		printf("Dados do triângulo superior:\n");
		for (int i = 0; i < size; i++) {
			printf("%.2f  ", triangle[i]);
		}
		printf("\n");
		free(triangle);
	}

	if (mat)
		free(mat);
	MPI_Type_free(&MPI_UpperTriangle);

	MPI_Finalize();
	return 0;
}