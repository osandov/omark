#include <string.h>
#include <stdio.h>
#include "params.h"

size_t block_size = 512;
bool block_aligned = false;
unsigned long initial_files = 1000;
size_t min_file_size = 1024;
size_t max_file_size = 100 * 1024;
size_t min_write_size = 512;
size_t max_write_size = 10 * 1024;
double io_dir_ratio = 0.75;
double read_write_ratio = 0.75;
double create_delete_ratio = 0.8;
unsigned long max_operations = 10000;
unsigned long time_limit = 0;

int parse_params(const char *config_path)
{
	FILE *file;
	char *line = NULL;
	size_t n = 0;
	ssize_t ret;
	char buf[6];

	if (strcmp(config_path, "-") == 0) {
		file = stdin;
	} else {
		file = fopen(config_path, "r");
		if (!file) {
			perror("fopen");
			return -1;
		}
	}

	while ((ret = getline(&line, &n, file)) >= 0) {
		sscanf(line, "block-size %zu\n", &block_size);
		if (sscanf(line, "block-aligned %5s", buf) == 1) {
			if (strcmp(buf, "true") == 0)
				block_aligned = true;
			else if (strcmp(buf, "false") == 0)
				block_aligned = false;
		}
		sscanf(line, "initial-files %lu", &initial_files);
		sscanf(line, "min-file-size %zu", &min_file_size);
		sscanf(line, "max-file-size %zu", &max_file_size);
		sscanf(line, "min-write-size %zu", &min_write_size);
		sscanf(line, "max-write-size %zu", &max_write_size);
		sscanf(line, "io-dir-ratio %lf", &io_dir_ratio);
		sscanf(line, "read-write-ratio %lf", &read_write_ratio);
		sscanf(line, "create-delete-ratio %lf", &create_delete_ratio);
		sscanf(line, "max-operations %lu", &max_operations);
		sscanf(line, "time-limit %lu", &time_limit);
	}

	if (file != stdin)
		fclose(file);
	return 0;
}

void dump_params(void)
{
	fprintf(stderr, "Benchmark parameters:\n");
	fprintf(stderr, "  block size=%zu\n", block_size);
	fprintf(stderr, "  block aligned=%s\n", block_aligned ? "true" : "false");
	fprintf(stderr, "  initial files=%ld\n", initial_files);
	fprintf(stderr, "  file size=%zu-%zu\n", min_file_size, max_file_size);
	fprintf(stderr, "  write size=%zu-%zu\n", min_write_size, max_write_size);
	fprintf(stderr, "  I/O operation/directory operation ratio=%f\n",
		io_dir_ratio);
	fprintf(stderr, "  read/write ratio=%f\n", read_write_ratio);
	fprintf(stderr, "  create/delete ratio=%f\n", create_delete_ratio);
	fprintf(stderr, "  max operations=%ld\n", max_operations);
	fprintf(stderr, "  time limit=%ld\n", time_limit);
}
