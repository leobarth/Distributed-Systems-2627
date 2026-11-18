#include <stdio.h>
#include <cuda_runtime.h>

__global__ void dot_product(const float *a, const float *b, float *result,
	int n) {
	// dynamically allocated shared memory per block
	extern __shared__ float sdata[];	

	int thread_id = threadIdx.x + blockIdx.x * blockDim.x;
	int cache_id = threadIdx.x;

	float sum = 0.0;
	while (thread_id < n) {
		sum += a[thread_id] * b[thread_id];
		thread_id += blockDim.x * gridDim.x;
	}
	sdata[cache_id] = sum;

	__syncthreads();

	// tree reduction in shared memory

	// note: I found that s /= 2 can also be written as s >>= 1, as dividing by
	// two means the same as shifting all bits by one position to the right
	for (int s = blockDim.x / 2; s > 0; s >>= 1) {
		if (cache_id < s) {
			sdata[cache_id] += sdata[cache_id + s];
		}
		__syncthreads();
	}

	if (cache_id == 0) {
		atomicAdd(result, sdata[0]);
	}
}

int main() {
	int n = 1 << 20; // roughly one million elements
	size_t bytes = n * sizeof(float);

	float *h_a = (float*)malloc(bytes);
	float *h_b = (float*)malloc(bytes);
	float h_result = 0.0;

	for (int i = 0; i < n; i++) {
		h_a[i] = 1.0;
		h_b[i] = 2.0;
	}

	float *d_a, *d_b, *d_result;
	cudaMalloc(&d_a, bytes);
	cudaMalloc(&d_b, bytes);
	cudaMalloc(&d_result, sizeof(float));

	cudaMemcpy(d_a, h_a, bytes, cudaMemcpyHostToDevice);
	cudaMemcpy(d_b, h_b, bytes, cudaMemcpyHostToDevice);
	cudaMemset(d_result, 0, sizeof(float));

	int threads_per_block = 256;
	int blocks_per_grid = min(65535, (n + threads_per_block - 1) /
		threads_per_block);
	size_t shared_mem_size = threads_per_block * sizeof(float);

	dot_product<<<blocks_per_grid, threads_per_block, shared_mem_size>>>
		(d_a, d_b, d_result, n);

	cudaMemcpy(&h_result, d_result, sizeof(float), cudaMemcpyDeviceToHost);
	printf("Dot product: %f", h_result);

	cudaFree(d_a);
	cudaFree(d_b);
	cudaFree(d_result);
	free(h_a);
	free(h_b);

	return 0;
}
