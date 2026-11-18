#include <stdio.h>
#include <cuda_runtime.h>

__global__ void dot_product(const float *a, const float *b, float *result,
	int n) {
	// dynamically allocated shared memory array per block
	extern __shared__ float shared_data[];

	int global_thread_id = threadIdx.x + blockIdx.x * blockDim.x;
	int local_thread_id = threadIdx.x;

	float sum = 0.0;
	while (global_thread_id < n) {
		sum += a[global_thread_id] * b[global_thread_id];
		global_thread_id += blockDim.x * gridDim.x;
	}
	shared_data[local_thread_id] = sum;

	__syncthreads();

	// tree reduction in shared memory

	// note: I found that s /= 2 can also be written as s >>= 1, as dividing by
	// two means the same as shifting all bits by one position to the right
	for (int s = blockDim.x / 2; s > 0; s >>= 1) {
		if (local_thread_id < s) {
			shared_data[local_thread_id] += shared_data[local_thread_id + s];
		}
		__syncthreads();
	}

	if (local_thread_id == 0) {
		atomicAdd(result, shared_data[0]);
	}
}

int main() {
	// 2 to the power of 20 entries (1048576) to demonstrate performance
	int n = 1 << 20;
	size_t bytes = n * sizeof(float);

	// host-side memory allocation
	float *h_a = (float*)malloc(bytes);
	float *h_b = (float*)malloc(bytes);
	float h_result = 0.0;

	// set test values; expected outcome for the dot product is 2097152.0
	for (int i = 0; i < n; i++) {
		h_a[i] = 1.0;
		h_b[i] = 2.0;
	}

	// device-side memory allocation
	float *d_a, *d_b, *d_result;
	cudaMalloc(&d_a, bytes);
	cudaMalloc(&d_b, bytes);
	cudaMalloc(&d_result, sizeof(float));

	// copy host-side data to device
	cudaMemcpy(d_a, h_a, bytes, cudaMemcpyHostToDevice);
	cudaMemcpy(d_b, h_b, bytes, cudaMemcpyHostToDevice);
	cudaMemset(d_result, 0, sizeof(float));

	// this value can be found as a good first experimental value in:
	// https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/index.html
	// (see 11.3: Thread and Block Heuristics)
	int threads_per_block = 256;
	// 65536 as a hard maximum, as I am not sure which graphics card is used;
	// this problem should not require this many blocks anyways
	int blocks_per_grid = min(65535, (n + threads_per_block - 1) /
		threads_per_block);
	// this parameter is crucial to put into the <<<>>> brackets when
	// initializing the kernel, because we are working with dynamic memory
	size_t shared_mem_size = threads_per_block * sizeof(float);

	dot_product<<<blocks_per_grid, threads_per_block, shared_mem_size>>>
		(d_a, d_b, d_result, n);

	// retrieve the results from device
	cudaMemcpy(&h_result, d_result, sizeof(float), cudaMemcpyDeviceToHost);
	printf("Dot product: %f\n", h_result);

	cudaFree(d_a);
	cudaFree(d_b);
	cudaFree(d_result);
	free(h_a);
	free(h_b);

	return 0;
}
