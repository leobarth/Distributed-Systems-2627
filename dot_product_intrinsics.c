#include <stdio.h>
#include <stdlib.h>
#include <x86intrin.h>

// this runs on my AMD CPU with x86_64 architecture
// compile with -mavx512f -mavx512vl -mfma -O3 or -march=native
float scalarp(const float *a, const float *b, int n) {
	// acc carries the partial sum through the loop for the vector reduction
	__m256 acc = _mm256_setzero_ps();
	size_t i = 0;

	for (; i + 8 <= n; i += 8) {
		__m256 va = _mm256_loadu_ps(a + i);
		__m256 vb = _mm256_loadu_ps(b + i);
		// fused multiply add (FMA) to compute the dot product:
		acc = _mm256_fmadd_ps(va, vb, acc);
	}

	// concatenate the 8 partial sums in acc to one scalar float value
	// first split the YMM-256-bit acc into two XMM-128-bit halves:
	__m128 lo = _mm256_castps256_ps128(acc);
	__m128 hi = _mm256_extractf128_ps(acc, 1);

	// lane-wise addition of the two halves:
	__m128 sum = _mm_add_ps(lo, hi);

	// horizontal add (HADD: all neighbouring pairs of two are added together)
	// twice:
	sum = _mm_hadd_ps(sum, sum);
	sum = _mm_hadd_ps(sum, sum);

	// extract lane 0 as our result:
	float result = _mm_cvtss_f32(sum);

	// tail value handling:
	for (; i < n; i++) {
		result += a[i] * b[i];
	}

	return result;
}

int main() {
    int n = 10;
    float a[10] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
    float b[10] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

    float result = scalarp(a, b, n);

    printf("The dot-product evaluates to: %.2f\n", result);

    return 0;
}
