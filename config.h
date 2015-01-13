/*
 * Benchmark configuration.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

/* I/O block size. */
extern size_t block_size;
/* Should all I/O be block-aligned? */
extern bool block_aligned;
/* Number of files to create before the benchmark runs. */
extern long initial_files;
/* Minimum/maximum initial file size. */
extern size_t min_file_size, max_file_size;
/* Minimum/maximum file write operation sizes. */
extern size_t min_write_size, max_write_size;
/* Ratio of I/O (read/write) to directory (create/delete) operations. */
extern double io_dir_ratio;
/* Ratio of reads to writes. */
extern double read_write_ratio;
/* Ratio of creates to deletes. */
extern double create_delete_ratio;
/* Maximum number of operations (0 means no limit). */
extern long max_operations;
/* Maximum number of seconds to run (0 means no limit). */
extern long time_limit;

int parse_config(const char *config_path);

void dump_config(void);

#endif /* CONFIG_H */
