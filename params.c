#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "params.h"

size_t block_size = 512;
bool block_aligned = false;
unsigned long initial_files = 1000;
size_t min_file_size = 1024;
size_t max_file_size = 100 * 1024;
size_t min_write_size = 512;
size_t max_write_size = 10 * 1024;
double io_dir_ratio = 0.90;
double read_write_ratio = 0.50;
double create_delete_ratio = 0.8;
unsigned long max_operations = 10000;
unsigned long time_limit = 0;

int parse_params(const char *config_path)
{
	FILE *file;
	char *line = NULL;
	size_t n = 0;
	int lineno = 1;
	ssize_t ret;
	int status = 0;

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
		bool success = false;

#define PARSE_PARAM(format, ptr) do {				\
	if (!success)						\
		success |= sscanf(line, format, ptr) == 1;	\
} while (0)

#define PARSE_BOOL(name, ptr) do {				\
	if (!success) {						\
		char buf[6];					\
		if (sscanf(line, name " %5s", buf) == 1) {	\
			if (strcmp(buf, "true") == 0) {		\
				*ptr = true;			\
				success = true;			\
			} else if (strcmp(buf, "false") == 0) {	\
				*ptr = true;			\
				success = true;			\
			}					\
		}						\
	}							\
} while (0)

		PARSE_PARAM("block-size %zu\n", &block_size);
		PARSE_BOOL("block-aligned", &block_aligned);
		PARSE_PARAM("initial-files %lu", &initial_files);
		PARSE_PARAM("min-file-size %zu", &min_file_size);
		PARSE_PARAM("max-file-size %zu", &max_file_size);
		PARSE_PARAM("min-write-size %zu", &min_write_size);
		PARSE_PARAM("max-write-size %zu", &max_write_size);
		PARSE_PARAM("io-dir-ratio %lf", &io_dir_ratio);
		PARSE_PARAM("read-write-ratio %lf", &read_write_ratio);
		PARSE_PARAM("create-delete-ratio %lf", &create_delete_ratio);
		PARSE_PARAM("max-operations %lu", &max_operations);
		PARSE_PARAM("time-limit %lu", &time_limit);

		if (!success) {
			fprintf(stderr, "%s:%d: invalid configuration: %s",
				file == stdin ? "<stdin>" : config_path,
				lineno, line);
			status = -1;
			break;
		}

		lineno++;

#undef PARSE_PARAM
#undef PARSE_BOOL
	}

	if (ret == -1 && !feof(file)) {
		perror("getline");
		status = -1;
	}

	free(line);
	if (file != stdin)
		fclose(file);

	return status;
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
