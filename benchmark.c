#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "benchmark.h"
#include "config.h"
#include "prng.h"

ssize_t read_full(int fd, void *buf, size_t count)
{
	ssize_t total_read = 0;

	while (count > 0) {
		ssize_t ret;

		ret = read(fd, buf, count);
		if (ret == -1)
			return -1;
		else if (ret == 0)
			break;
		total_read += ret;
		count -= ret;
	}

	return total_read;
}

ssize_t write_full(int fd, void *buf, size_t count)
{
	ssize_t total_written = 0;

	while (count > 0) {
		ssize_t ret;

		ret = write(fd, buf, count);
		if (ret == -1)
			return -1;
		total_written += ret;
		count -= ret;
	}

	return total_written;
}

int create_initial_files(void)
{
	char path[NAME_MAX];
	int fd;
	ssize_t ret;
	uint32_t num_blocks;
	char *buffer;

	buffer = malloc(block_size);
	if (!buffer) {
		perror("malloc");
		return -1;
	}

	for (long i = 0; i < initial_files; i++) {
		snprintf(path, sizeof(path), "%ld", i);

		fd = open(path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
		if (fd == -1) {
			perror("open");
			free(buffer);
			return -1;
		}

		num_blocks = prng_range(initial_file_min_size,
					initial_file_max_size + 1) / block_size;
		for (uint32_t i = 0; i < num_blocks; i++) {
			prng_bytes(buffer, block_size);
			ret = write_full(fd, buffer, block_size);
			if (ret == -1) {
				perror("write");
				free(buffer);
				return -1;
			}
		}
	}

	free(buffer);

	return 0;
}

static void do_read(struct benchmark_results *results)
{
	results->read_operations++;
}

static void do_write(struct benchmark_results *results)
{
	results->write_operations++;
}

static void do_create(struct benchmark_results *results)
{
	results->create_operations++;
}

static void do_delete(struct benchmark_results *results)
{
	results->delete_operations++;
}

void run_benchmark(struct benchmark_results *results)
{
	long i = 0;

	memset(results, 0, sizeof(*results));

	clock_gettime(CLOCK_MONOTONIC, &results->start_time);

	while (max_operations == 0 || i++ < max_operations) {
		if (prng_bool(io_dir_ratio)) {
			if (prng_bool(read_write_ratio))
				do_read(results);
			else
				do_write(results);
		} else {
			if (prng_bool(create_delete_ratio))
				do_create(results);
			else
				do_delete(results);
		}
	}

	clock_gettime(CLOCK_MONOTONIC, &results->end_time);
}
