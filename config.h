/*
 * Benchmark configuration.
 */

#ifndef CONFIG_H
#define CONFIG_H

/* I/O block size. */
extern size_t block_size;
/* Number of files to create before the benchmark runs. */
extern long initial_files;
/* Minimum/maximum initial file size. */
extern size_t initial_file_min_size, initial_file_max_size;
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
