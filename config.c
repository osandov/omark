#include <jansson.h>
#include <stdio.h>

long initial_files = 1000;
double read_write_ratio = 0.5;
double create_delete_ratio = 0.5;
long max_operations = 10000;
long time_limit = 0;

static void config_get_integer(json_t *config, const char *key, long *dest)
{
	const json_t *object;

	object = json_object_get(config, key);
	if (json_is_integer(object))
		*dest = json_integer_value(object);
}

static void config_get_float(json_t *config, const char *key, double *dest)
{
	const json_t *object;

	object = json_object_get(config, key);
	if (json_is_real(object))
		*dest = json_real_value(object);
}

int parse_config(const char *config_path)
{
	json_t *config;
	json_error_t error;

	config = json_load_file(config_path, 0, &error);
	if (!config) {
		fprintf(stderr, "%s:%d:%d: %s\n", error.source, error.line,
			error.column, error.text);
		return -1;
	}

	config_get_integer(config, "initial-files", &initial_files);
	config_get_float(config, "read-write-ratio", &read_write_ratio);
	config_get_float(config, "create-delete-ratio", &create_delete_ratio);
	config_get_integer(config, "max-operations", &max_operations);
	config_get_integer(config, "time-limit", &time_limit);

	json_decref(config);
	return 0;
}

void dump_config(void)
{
	fprintf(stderr, "Configuration:\n");
	fprintf(stderr, "  initial files=%ld\n", initial_files);
	fprintf(stderr, "  read/write ratio=%f\n", read_write_ratio);
	fprintf(stderr, "  create/delete ratio=%f\n", create_delete_ratio);
	fprintf(stderr, "  max operations=%ld\n", max_operations);
	fprintf(stderr, "  time limit=%ld\n", time_limit);
}
