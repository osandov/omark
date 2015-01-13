/*
 * Benchmark configuration.
 */

#ifndef CONFIG_H
#define CONFIG_H

/* Number of files to create before the benchmark runs. */
extern long initial_files;
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
