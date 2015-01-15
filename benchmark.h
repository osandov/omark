#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "prng.h"

struct benchmark_results {
	struct timespec elapsed_time;

	long read_operations;
	long write_operations;
	long create_operations;
	long delete_operations;

	size_t bytes_read;
	size_t bytes_written;
};

struct benchmark_thread {
	pthread_t thread;
	struct benchmark_results results;
	uint32_t prng_seed;
	struct prng prng;
	char *buffer;
};

extern pthread_barrier_t barrier;

/**
 * init_benchmark_files - create initial set of files
 */
int init_benchmark_files(uint32_t prng_seed);

/**
 * run_benchmark - run the benchmark
 */
void *run_benchmark(void *arg);

#endif /* BENCHMARK_H */
