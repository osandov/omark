#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config.h"

static const char *progname;

static void usage(bool error)
{
	FILE *file = error ? stderr : stdout;

	fprintf(file,
		"Usage: %1$s -c CONFIG\n"
		"       %1$s -h\n"
		"\n"
		"Filesystem benchmark.\n",
		progname);

	exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	int opt;
	char *config_path = NULL;
	int ret;

	progname = argv[0];

	while ((opt = getopt(argc, argv, "c:h")) != -1) {
		switch (opt) {
		case 'c':
			free(config_path);
			config_path = strdup(optarg);
			if (!config_path) {
				perror("strdup");
				return EXIT_FAILURE;
			}
			break;
		case 'h':
			usage(false);
		default:
			usage(true);
		}
	}

	if (!config_path)
		usage(true);
	ret = parse_config(config_path);
	free(config_path);
	if (ret)
		return EXIT_FAILURE;
	dump_config();

	return EXIT_SUCCESS;
}
