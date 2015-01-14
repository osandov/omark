#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "benchmark.h"
#include "params.h"
#include "prng.h"

static const char *progname;

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

static void usage(bool error)
{
	FILE *file = error ? stderr : stdout;

	fprintf(file,
		"Usage: %1$s [-c CONFIG] [-C DIR] [-d]\n"
		"       %1$s -h\n"
		"\n"
		"Filesystem benchmark.\n"
		"\n"
		"Configuration:\n"
		"  -C DIR       Change directories before running\n"
		"  -c CONFIG    Benchmark configuration file\n"
		"  -d           Dump benchmark configuration\n"
		"\n"
		"Miscellaneous:\n"
		"  -h           Display this help message and exit\n",
		progname);

	exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	int opt;
	char chdir_path[PATH_MAX];
	char config_path[PATH_MAX];
	bool chdir_path_flag = false;
	bool config_path_flag = false;
	bool dump_params_flag = false;

	int ret;
	struct benchmark_results results;

	progname = argv[0];

	while ((opt = getopt(argc, argv, "C:c:dh")) != -1) {
		switch (opt) {
		case 'C':
			strncpy(chdir_path, optarg, sizeof(chdir_path) - 1);
			chdir_path[sizeof(chdir_path) - 1] = '\0';
			chdir_path_flag = true;
			break;
		case 'c':
			strncpy(config_path, optarg, sizeof(config_path) - 1);
			config_path[sizeof(config_path) - 1] = '\0';
			config_path_flag = true;
			break;
		case 'd':
			dump_params_flag = true;
			break;
		case 'h':
			usage(false);
		default:
			usage(true);
		}
	}

	if (config_path_flag) {
		ret = parse_params(config_path);
		if (ret)
			return EXIT_FAILURE;
	}

	if (dump_params_flag) {
		dump_params();
		return EXIT_SUCCESS;
	}

	if (chdir_path_flag) {
		ret = chdir(chdir_path);
		if (ret) {
			perror("chdir");
			return ret;
		}
	}

	prng_seed(0xdeadbeef);

	ret = init_benchmark();
	if (ret)
		return EXIT_FAILURE;

	run_benchmark(&results);

	verbose_report(&results);

	return EXIT_SUCCESS;
}
