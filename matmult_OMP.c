// parallelized matrix multiplication A*B=C using OpenMP
// A has dimensions MxK, B has dimensions KxN, C therefore MxN

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define M 800
#define K 1000
#define N 700

void matmult_omp(const double *a, const double *b, double *c, int m, int k, int n) {
	// sharing variables a, b, c to calculate in a parallelized manner
	#pragma omp parallel for shared(a, b, c)
	for (int i = 0; i < m; i++) {
		for (int j = 0; j < n; j++) {
			double sum = 0.0;
			for (int k_iter = 0; k_iter < k; k_iter++) {
				sum += a[i*k + k_iter] * b[k_iter*n + j];
			}
			c[i*n + j] = sum;
		}
	}
}

int main() {
	int m = M;
	int k = K;
	int n = N;

	size_t bytes_A = (size_t)m * k * sizeof(double);
	size_t bytes_B = (size_t)k * n * sizeof(double);
	size_t bytes_C = (size_t)m * n * sizeof(double);

	double *A = (double *)malloc(bytes_A);
	double *B = (double *)malloc(bytes_B);
	double *C = (double *)malloc(bytes_C);

	// set test values, expecting outcome C with entry 2.0 * k everywhere

	#pragma omp parallel for
	for (int i = 0; i < m; i++) {
		for (int j = 0; j < k; j++) {
			A[i*k + j] = 1.0;
		}
	}

	#pragma omp parallel for
	for (int i = 0; i < k; i++) {
		for (int j = 0; j < n; j++) {
			B[i*n + j] = 2.0;
		}
	}

	double start_time = omp_get_wtime();
	matmult_omp(A, B, C, m, k, n);
	double end_time = omp_get_wtime();

	printf("Multiplication done:\n");
	printf("Time taken: %fs\n", end_time - start_time);
	printf("Example value C[0][0]: %f (expected: %f)\n", C[0], (double)k * 2.0);

	free(A);
	free(B);
	free(C);

	return 0;
}
