#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "benchmark.h"
#include "config.h"
#include "prng.h"

static const char *progname;

static void usage(bool error)
{
	FILE *file = error ? stderr : stdout;

	fprintf(file,
		"Usage: %1$s [-c CONFIG] [-d]\n"
		"       %1$s -h\n"
		"\n"
		"Filesystem benchmark.\n",
		progname);

	exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}

static void timespec_subtract(struct timespec *result, const struct timespec *x,
			      const struct timespec *y)
{
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_nsec = x->tv_nsec - y->tv_nsec;
	if (result->tv_nsec < 0) {
		result->tv_nsec += 1000000000L;
		result->tv_sec--;
	}
}

static void print_human_readable_bytes(double bytes, int precision)
{
	static const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
	int i = 0;
	while (i < sizeof(units) / sizeof(units[0]) - 1 && bytes >= 1024.0) {
		bytes /= 1024.0;
		i++;
	}
	printf("%.*f%s", precision, bytes, units[i]);
}

static void verbose_report(const struct benchmark_results *results)
{
	struct timespec elapsed;
	double elapsed_secs;
	long total_operations, io_operations, dir_operations;

	timespec_subtract(&elapsed, &results->end_time, &results->start_time);
	elapsed_secs = elapsed.tv_sec + elapsed.tv_nsec / 1000000000.0;

	printf("Elapsed time: %lld.%.9ldsec\n", (long long)elapsed.tv_sec,
	       elapsed.tv_nsec);

	printf("\n");

	io_operations = results->read_operations + results->write_operations;
	dir_operations = results->create_operations + results->delete_operations;
	total_operations = io_operations + dir_operations;

	printf("Total operations: %ld (%.2f/sec)\n",
	       total_operations, total_operations / elapsed_secs);

	printf("I/O (read/write) operations: %ld (%.1f%%, %.2f/sec)\n",
	       io_operations,
	       100.0 * ((double)io_operations / (double)total_operations),
	       io_operations / elapsed_secs);

	printf("Directory (create/delete) operations: %ld (%.1f%%, %.2f/sec)\n",
	       dir_operations,
	       100.0 * ((double)dir_operations / (double)total_operations),
	       dir_operations / elapsed_secs);

	printf("\n");

	printf("Read operations: %ld (%.1f%% total, %.1f%% read/write, %.2f/sec)\n",
	       results->read_operations,
	       100.0 * ((double)results->read_operations / (double)total_operations),
	       100.0 * ((double)results->read_operations / (double)io_operations),
	       results->read_operations / elapsed_secs);

	printf("Write operations: %ld (%.1f%% total, %.1f%% read/write, %.2f/sec)\n",
	       results->write_operations,
	       100.0 * ((double)results->write_operations / (double)total_operations),
	       100.0 * ((double)results->write_operations / (double)io_operations),
	       results->write_operations / elapsed_secs);

	printf("Create operations: %ld (%.1f%% total, %.1f%% create/delete %.2f/sec)\n",
	       results->create_operations,
	       100.0 * ((double)results->create_operations / (double)total_operations),
	       100.0 * ((double)results->create_operations / (double)dir_operations),
	       results->create_operations / elapsed_secs);

	printf("Delete operations: %ld (%.1f%% total, %.1f%% create/delete, %.2f/sec)\n",
	       results->delete_operations,
	       100.0 * ((double)results->delete_operations / (double)total_operations),
	       100.0 * ((double)results->delete_operations / (double)dir_operations),
	       results->delete_operations / elapsed_secs);

	printf("\n");

	printf("Read ");
	print_human_readable_bytes(results->bytes_read, 2);
	printf(" (");
	print_human_readable_bytes(results->bytes_read / elapsed_secs, 2);
	printf("/s)\n");

	printf("Wrote ");
	print_human_readable_bytes(results->bytes_written, 2);
	printf(" (");
	print_human_readable_bytes(results->bytes_written / elapsed_secs, 2);
	printf("/s)\n");
}

int main(int argc, char *argv[])
{
	int opt;
	char *config_path = NULL;
	bool dump_config_flag = false;

	int ret;
	struct benchmark_results results;

	progname = argv[0];

	while ((opt = getopt(argc, argv, "c:dh")) != -1) {
		switch (opt) {
		case 'c':
			free(config_path);
			config_path = strdup(optarg);
			if (!config_path) {
				perror("strdup");
				return EXIT_FAILURE;
			}
			break;
		case 'd':
			dump_config_flag = true;
			break;
		case 'h':
			usage(false);
		default:
			usage(true);
		}
	}

	if (config_path) {
		ret = parse_config(config_path);
		free(config_path);
		if (ret)
			return EXIT_FAILURE;
	}

	if (dump_config_flag) {
		dump_config();
		return EXIT_SUCCESS;
	}

	prng_seed(0xdeadbeef);

	ret = init_benchmark();
	if (ret)
		return EXIT_FAILURE;

	run_benchmark(&results);

	verbose_report(&results);

	return EXIT_SUCCESS;
}
