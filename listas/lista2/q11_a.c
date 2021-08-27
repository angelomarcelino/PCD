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

	pSum[0] = arr[0];

	for (int i = 1; i < size; i++) {
		pSum[i] = pSum[i - 1] + arr[i];
	}

	printf("Prefix Sum:\n\t");
	for (int i = 0; i < size; i++) {
		printf("%d  ", pSum[i]);
	}
	printf("\n");
	return 0;
}