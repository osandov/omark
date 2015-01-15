#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "benchmark.h"
#include "params.h"
#include "prng.h"

static const char *progname;
static struct benchmark_thread *threads;
static int num_threads = 1;

static void print_human_readable_bytes(double bytes, int precision)
{
	static const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
	int i = 0;
	while (i < sizeof(units) / sizeof(units[0]) - 1 && bytes >= 1024.0) {
		bytes /= 1024.0;
		i++;
	}
	printf("%.*f %s", precision, bytes, units[i]);
}

static void verbose_print_results(const struct benchmark_results *results,
				  double elapsed_secs)
{
	long total_operations, io_operations, dir_operations;

	io_operations = results->read_operations + results->write_operations;
	dir_operations = results->create_operations + results->delete_operations;
	total_operations = io_operations + dir_operations;

	printf("  Total operations: %ld (%.2f/sec)\n",
	       total_operations, total_operations / elapsed_secs);

	printf("  I/O (read/write) operations: %ld (%.1f%%, %.2f/sec)\n",
	       io_operations,
	       100.0 * ((double)io_operations / (double)total_operations),
	       io_operations / elapsed_secs);

	printf("  Directory (create/delete) operations: %ld (%.1f%%, %.2f/sec)\n",
	       dir_operations,
	       100.0 * ((double)dir_operations / (double)total_operations),
	       dir_operations / elapsed_secs);

	printf("\n");

	printf("  Read operations: %ld (%.1f%% total, %.1f%% read/write, %.2f/sec)\n",
	       results->read_operations,
	       100.0 * ((double)results->read_operations / (double)total_operations),
	       100.0 * ((double)results->read_operations / (double)io_operations),
	       results->read_operations / elapsed_secs);

	printf("  Write operations: %ld (%.1f%% total, %.1f%% read/write, %.2f/sec)\n",
	       results->write_operations,
	       100.0 * ((double)results->write_operations / (double)total_operations),
	       100.0 * ((double)results->write_operations / (double)io_operations),
	       results->write_operations / elapsed_secs);

	printf("  Create operations: %ld (%.1f%% total, %.1f%% create/delete %.2f/sec)\n",
	       results->create_operations,
	       100.0 * ((double)results->create_operations / (double)total_operations),
	       100.0 * ((double)results->create_operations / (double)dir_operations),
	       results->create_operations / elapsed_secs);

	printf("  Delete operations: %ld (%.1f%% total, %.1f%% create/delete, %.2f/sec)\n",
	       results->delete_operations,
	       100.0 * ((double)results->delete_operations / (double)total_operations),
	       100.0 * ((double)results->delete_operations / (double)dir_operations),
	       results->delete_operations / elapsed_secs);

	printf("\n");

	printf("  Read ");
	print_human_readable_bytes(results->bytes_read, 2);
	printf(" (");
	print_human_readable_bytes(results->bytes_read / elapsed_secs, 2);
	printf("/s)\n");

	printf("  Wrote ");
	print_human_readable_bytes(results->bytes_written, 2);
	printf(" (");
	print_human_readable_bytes(results->bytes_written / elapsed_secs, 2);
	printf("/s)\n");
}

static void verbose_thread(int i)
{
	const struct benchmark_results *results;
	double elapsed_secs;

	results = &threads[i].results;

	elapsed_secs = (results->elapsed_time.tv_sec +
			results->elapsed_time.tv_nsec / 1000000000.0);

	printf("Thread %d:\n", i);

	printf("  Elapsed time: %lld.%.9ld sec\n",
	       (long long)results->elapsed_time.tv_sec,
	       results->elapsed_time.tv_nsec);

	printf("\n");

	verbose_print_results(results, elapsed_secs);
}

static void verbose_total(const struct benchmark_results *results)
{
	double elapsed_secs, avg_elapsed_secs;

	elapsed_secs = (results->elapsed_time.tv_sec +
			results->elapsed_time.tv_nsec / 1000000000.0);
	avg_elapsed_secs = elapsed_secs / num_threads;

	printf("\nTotal:\n");
	printf("  Total elapsed time: %.9f sec\n", elapsed_secs);
	printf("  Average elapsed time: %.9f sec\n", avg_elapsed_secs);
	printf("\n");

	verbose_print_results(results, avg_elapsed_secs);
}

static void verbose_report(void)
{
	struct benchmark_results total_results = {};

	for (int i = 0; i < num_threads; i++) {
		if (i > 0)
			printf("\n");

		verbose_thread(i);

		total_results.read_operations += threads[i].results.read_operations;
		total_results.write_operations += threads[i].results.write_operations;
		total_results.create_operations += threads[i].results.create_operations;
		total_results.delete_operations += threads[i].results.delete_operations;

		total_results.bytes_read += threads[i].results.bytes_read;
		total_results.bytes_written += threads[i].results.bytes_written;

		total_results.elapsed_time.tv_sec +=
			threads[i].results.elapsed_time.tv_sec;
		total_results.elapsed_time.tv_nsec +=
			threads[i].results.elapsed_time.tv_nsec;
		if (total_results.elapsed_time.tv_nsec >= 1000000000L) {
			total_results.elapsed_time.tv_nsec -= 1000000000L;
			total_results.elapsed_time.tv_sec++;
		}
	}

	if (num_threads > 1)
		verbose_total(&total_results);
}

#define OPTS EXTRA_OPTS

static void usage(bool error)
{
	FILE *file = error ? stderr : stdout;

	fprintf(file,
		"Usage: %1$s [OPTIONS]\n"
		"\n"
		"Filesystem benchmark.\n"
		"\n"
		"Configuration:\n"
		"  -C DIR       Change directories before running\n"
		"  -c CONFIG    Benchmark configuration file\n"
		"  -d           Dump benchmark configuration\n"
		"  -p THREADS   Run multiple threads in parallel\n"
		"  -s SEED      PRNG seed value\n"
		"\n"
		"Miscellaneous:\n"
		"  -h           Display this help message and exit\n",
		progname);

	exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	int ret;

	int opt;
	char *end;
	char *chdir_path;
	char *config_path;
	long seed = 0xdeadbeefL;
	bool dump_params_flag = false;

	progname = argv[0];

	while ((opt = getopt(argc, argv, "C:c:dp:s:h")) != -1) {
		switch (opt) {
		case 'C':
			chdir_path = strdup(optarg);
			if (!chdir_path) {
				perror("strdup");
				return EXIT_FAILURE;
			}
			break;
		case 'c':
			config_path = strdup(optarg);
			if (!config_path) {
				perror("strdup");
				return EXIT_FAILURE;
			}
			break;
		case 'd':
			dump_params_flag = true;
			break;
		case 's':
			seed = strtol(optarg, &end, 0);
			if (*end != '\0') {
				fprintf(stderr, "%s: invalid PRNG seed\n",
					progname);
				return EXIT_FAILURE;
			}
			break;
		case 'p':
			num_threads = strtol(optarg, &end, 10);
			if (num_threads == 0 || *end != '\0') {
				fprintf(stderr, "%s: invalid number of threads\n",
					progname);
				return EXIT_FAILURE;
			}
			break;
		case 'h':
			usage(false);
		default:
			usage(true);
		}
	}

	if (config_path) {
		ret = parse_params(config_path);
		if (ret)
			return EXIT_FAILURE;
		free(config_path);
	}

	if (dump_params_flag) {
		dump_params();
		return EXIT_SUCCESS;
	}

	if (chdir_path) {
		ret = chdir(chdir_path);
		if (ret) {
			perror("chdir");
			return EXIT_FAILURE;
		}
		free(chdir_path);
	}

	threads = calloc(num_threads, sizeof(threads[0]));
	if (!threads) {
		perror("calloc");
		return EXIT_FAILURE;
	}
	errno = pthread_barrier_init(&barrier, NULL, num_threads);
	if (errno) {
		perror("pthread_barrier_init");
		return EXIT_FAILURE;
	}

	for (int i = 0; i < num_threads; i++) {
		threads[i].prng_seed = seed + i;
		threads[i].buffer = malloc(block_size);
		if (!threads[i].buffer) {
			perror("malloc");
			return EXIT_FAILURE;
		}
	}

	ret = init_benchmark_files(seed - 1);
	if (ret)
		return EXIT_FAILURE;

	for (int i = 0; i < num_threads; i++) {
		errno = pthread_create(&threads[i].thread, NULL, run_benchmark,
				       &threads[i]);
		if (errno != 0) {
			perror("pthread_create");
			return EXIT_FAILURE;
		}
	}

	for (int i = 0; i < num_threads; i++) {
		void *retval;

		pthread_join(threads[i].thread, &retval);
		if (retval) {
			fprintf(stderr, "%s: thread %d failed\n", progname, i);
			return EXIT_FAILURE;
		}
	}

	verbose_report();

	return EXIT_SUCCESS;
}
