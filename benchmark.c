#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "benchmark.h"
#include "params.h"
#include "prng.h"

pthread_barrier_t barrier;

/* Atomic counters. */
static long num_operations;
static long path_counter;

static pthread_rwlock_t files_lock = PTHREAD_RWLOCK_INITIALIZER;
static long *files_array;
static size_t files_size, files_capacity;

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
		buf = (char *)buf + ret;
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
		buf = (char *)buf + ret;
		total_written += ret;
		count -= ret;
	}

	return total_written;
}

static int write_to_file(struct benchmark_thread *thread, int fd, size_t size)
{
	ssize_t ret;

	if (block_aligned)
		size = size - (size % block_size);

	while (size > block_size) {
		prng_bytes(&thread->prng, thread->buffer, block_size);
		ret = write_full(fd, thread->buffer, block_size);
		if (ret == -1) {
			perror("write");
			return -1;
		}
		size -= block_size;
	}

	prng_bytes(&thread->prng, thread->buffer, size);
	ret = write_full(fd, thread->buffer, size);
	if (ret == -1) {
		perror("write");
		return -1;
	}

	return 0;
}

static int create_file(struct benchmark_thread *thread)
{
	char path[NAME_MAX];
	long path_num;
	int fd;
	size_t size;
	int ret;

	path_num = __atomic_fetch_add(&path_counter, 1, __ATOMIC_SEQ_CST);
	snprintf(path, sizeof(path), "%ld", path_num);

	fd = open(path, O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		perror("open");
		return -1;
	}

	size = prng_range(&thread->prng, min_file_size, max_file_size + 1);
	ret = write_to_file(thread, fd, size);
	close(fd);
	if (ret == -1)
		return -1;

	thread->results.bytes_written += size;

	pthread_rwlock_wrlock(&files_lock);
	if (files_size >= files_capacity) {
		long *new_array;
		size_t new_capacity;

		new_capacity = files_capacity * 2 + 1;
		new_array = realloc(files_array,
				    sizeof(files_array[0]) * new_capacity);
		if (!new_array) {
			perror("realloc");
			pthread_rwlock_unlock(&files_lock);
			return -1;
		}

		files_array = new_array;
		files_capacity = new_capacity;
	}
	files_array[files_size++] = path_num;
	pthread_rwlock_unlock(&files_lock);

	return 0;
}

/* files_lock must be held. */
static int pick_file(struct benchmark_thread *thread, char *name_ret,
		     uint32_t *index_ret)
{
	uint32_t index;

	if (files_size == 0)
		return -1;

	index = prng_range(&thread->prng, 0, files_size);
	snprintf(name_ret, NAME_MAX, "%ld", files_array[index]);
	if (index_ret)
		*index_ret = index;

	return 0;
}

static void do_read(struct benchmark_thread *thread)
{
	char path[NAME_MAX];
	int fd;
	ssize_t ret;

	pthread_rwlock_rdlock(&files_lock);
	if (pick_file(thread, path, NULL) == -1) {
		pthread_rwlock_unlock(&files_lock);
		return;
	}

	fd = open(path, O_RDONLY);
	pthread_rwlock_unlock(&files_lock);
	if (fd == -1) {
		perror("open");
		return;
	}

	while ((ret = read_full(fd, thread->buffer, block_size)) > 0)
		thread->results.bytes_read += ret;
	if (ret == -1) {
		perror("read");
		close(fd);
		return;
	}

	close(fd);
	thread->results.read_operations++;
}

static void do_write(struct benchmark_thread *thread)
{
	char path[NAME_MAX];
	int fd;
	size_t size;
	int ret;

	pthread_rwlock_rdlock(&files_lock);
	if (pick_file(thread, path, NULL) == -1) {
		pthread_rwlock_unlock(&files_lock);
		return;
	}

	fd = open(path, O_WRONLY | O_APPEND);
	pthread_rwlock_unlock(&files_lock);
	if (fd == -1) {
		perror("open");
		return;
	}

	size = prng_range(&thread->prng, min_write_size, max_write_size + 1);
	ret = write_to_file(thread, fd, size);
	close(fd);
	if (ret == -1)
		return;

	thread->results.bytes_written += size;
	thread->results.write_operations++;
}

static void do_create(struct benchmark_thread *thread)
{
	if (create_file(thread) == -1)
		return;

	thread->results.create_operations++;
}

static void do_delete(struct benchmark_thread *thread)
{
	char path[NAME_MAX];
	uint32_t index;

	pthread_rwlock_wrlock(&files_lock);
	if (pick_file(thread, path, &index) == -1) {
		pthread_rwlock_unlock(&files_lock);
		return;
	}

	files_size--;
	memmove(files_array + index, files_array + index + 1,
		(files_size - index) * sizeof(files_array[0]));
	pthread_rwlock_unlock(&files_lock);

	if (unlink(path) == -1)
		perror("unlink");
	else
		thread->results.delete_operations++;
}

int init_benchmark_files(uint32_t prng_seed)
{
	struct benchmark_thread dummy_thread;
	int ret;

	prng_init(&dummy_thread.prng, prng_seed);
	dummy_thread.buffer = malloc(block_size);
	if (!dummy_thread.buffer) {
		perror("malloc");
		return -1;
	}

	for (long i = 0; i < initial_files; i++) {
		ret = create_file(&dummy_thread);
		if (ret)
			return -1;
	}

	free(dummy_thread.buffer);
	return 0;
}

void uninit_benchmark(void)
{
	free(files_array);
}

static inline void timespec_subtract(struct timespec *restrict result,
				     const struct timespec *restrict x,
				     const struct timespec *restrict y)
{
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_nsec = x->tv_nsec - y->tv_nsec;
	if (result->tv_nsec < 0) {
		result->tv_nsec += 1000000000L;
		result->tv_sec--;
	}
}

void *run_benchmark(void *arg)
{
	struct benchmark_thread *thread = arg;
	struct timespec start_time, end_time, elapsed_time;

	prng_init(&thread->prng, thread->prng_seed);

	pthread_barrier_wait(&barrier);

	clock_gettime(CLOCK_MONOTONIC, &start_time);

	for (;;) {
		if (time_limit) {
			clock_gettime(CLOCK_MONOTONIC, &end_time);
			timespec_subtract(&elapsed_time, &end_time, &start_time);
			if (elapsed_time.tv_sec >= time_limit)
				break;
		}

		if (max_operations) {
			long ops = __atomic_fetch_add(&num_operations, 1,
						      __ATOMIC_SEQ_CST);
			if (ops >= max_operations)
				break;
		}

		if (prng_bool(&thread->prng, io_dir_ratio)) {
			if (prng_bool(&thread->prng, read_write_ratio))
				do_read(thread);
			else
				do_write(thread);
		} else {
			if (prng_bool(&thread->prng, create_delete_ratio))
				do_create(thread);
			else
				do_delete(thread);
		}
	}

	clock_gettime(CLOCK_MONOTONIC, &end_time);
	timespec_subtract(&thread->results.elapsed_time, &end_time, &start_time);

	return NULL;
}
