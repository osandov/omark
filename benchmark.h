#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <time.h>

struct benchmark_results {
	struct timespec start_time;
	struct timespec end_time;

	long read_operations;
	long write_operations;
	long create_operations;
	long delete_operations;

	size_t bytes_read;
	size_t bytes_written;
};

int create_initial_files(void);

void run_benchmark(struct benchmark_results *results);

#endif /* BENCHMARK_H */
