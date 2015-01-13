#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "benchmark.h"
#include "config.h"
#include "prng.h"

static sig_atomic_t time_is_up;

static long *files_array;
static size_t files_size, files_capacity;
static char *buffer;
static long path_counter;

ssize_t read_full(int fd, void *buf, size_t count)
{
	ssize_t total_read = 0;

	while (count > 0) {
		ssize_t ret;

		ret = read(fd, buf, count);
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			return -1;
		} else if (ret == 0) {
			break;
		}
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
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		total_written += ret;
		count -= ret;
	}

	return total_written;
}

static int write_to_file(int fd, size_t size)
{
	ssize_t ret;

	if (block_aligned)
		size = size - (size % block_size);

	while (size > block_size) {
		prng_bytes(buffer, block_size);
		ret = write_full(fd, buffer, block_size);
		if (ret == -1) {
			perror("write");
			return -1;
		}
		size -= block_size;
	}

	prng_bytes(buffer, size);
	ret = write_full(fd, buffer, size);
	if (ret == -1) {
		perror("write");
		return -1;
	}

	return 0;
}

static int create_file(struct benchmark_results *results)
{
	char path[NAME_MAX];
	long path_num;
	int fd;
	size_t size;
	int ret;

	path_num = path_counter++;
	snprintf(path, sizeof(path), "%ld", path_num);

	fd = open(path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		perror("open");
		free(buffer);
		return -1;
	}

	size = prng_range(min_file_size, max_file_size + 1);
	ret = write_to_file(fd, size);
	close(fd);
	if (ret == -1)
		return -1;

	if (results)
		results->bytes_written += size;

	if (files_size >= files_capacity) {
		long *new_array;
		size_t new_capacity;

		new_capacity = files_capacity * 2 + 1;
		new_array = realloc(files_array,
				    sizeof(files_array[0]) * new_capacity);
		if (!new_array) {
			perror("realloc");
			return -1;
		}

		files_array = new_array;
		files_capacity = new_capacity;
	}
	files_array[files_size++] = path_num;

	return 0;
}

static void timeout(int signum)
{
	time_is_up = 1;
}

int init_benchmark(void)
{
	struct sigaction sa;
	int ret;

	buffer = malloc(block_size);
	if (!buffer) {
		perror("malloc");
		return -1;
	}

	for (long i = 0; i < initial_files; i++) {
		ret = create_file(NULL);
		if (ret) {
			free(buffer);
			return -1;
		}
	}

	sa.sa_handler = timeout;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	ret = sigaction(SIGALRM, &sa, NULL);
	if (ret == -1) {
		perror("sigaction");
		return -1;
	}

	return 0;
}

static int pick_file(char *name_ret, uint32_t *index_ret)
{
	uint32_t index;

	if (files_size == 0)
		return -1;

	index = prng_range(0, files_size);
	snprintf(name_ret, NAME_MAX, "%ld", files_array[index]);
	if (index_ret)
		*index_ret = index;

	return 0;
}

static void do_read(struct benchmark_results *results)
{
	char path[NAME_MAX];
	int fd;
	ssize_t ret;

	if (pick_file(path, NULL) == -1)
		return;

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		perror("open");
		return;
	}

	while ((ret = read_full(fd, buffer, block_size)) > 0)
		results->bytes_read += ret;
	if (ret == -1) {
		perror("read");
		close(fd);
		return;
	}

	close(fd);
	results->read_operations++;
}

static void do_write(struct benchmark_results *results)
{
	char path[NAME_MAX];
	int fd;
	size_t size;
	int ret;

	if (pick_file(path, NULL) == -1)
		return;

	fd = open(path, O_WRONLY | O_APPEND);
	if (fd == -1) {
		perror("open");
		return;
	}

	size = prng_range(min_write_size, max_write_size + 1);
	ret = write_to_file(fd, size);
	close(fd);
	if (ret == -1)
		return;

	results->bytes_written += size;
	results->write_operations++;
}

static void do_create(struct benchmark_results *results)
{
	if (create_file(results) == -1)
		return;

	results->create_operations++;
}

static void do_delete(struct benchmark_results *results)
{
	char path[NAME_MAX];
	uint32_t index;

	if (pick_file(path, &index) == -1)
		return;

	memmove(files_array + index, files_array + index + 1,
		(files_size - index - 1) * sizeof(files_array[0]));
	files_size--;
	unlink(path);

	results->delete_operations++;
}

void run_benchmark(struct benchmark_results *results)
{
	long i = 0;

	memset(results, 0, sizeof(*results));

	if (time_limit)
		alarm(time_limit);

	clock_gettime(CLOCK_MONOTONIC, &results->start_time);

	while (!time_is_up && (max_operations == 0 || i++ < max_operations)) {
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

	free(buffer);
	free(files_array);
}
