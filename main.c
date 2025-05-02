#include <stdio.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

static void repl(void)
{
	char line[1024];

	for (;;) {
		printf("> ");
		if (!fgets(line, sizeof(line), stdin)) {
			printf("\n");
			break;
		}

		interpret(line);
	}
}

static char *read_file(const char *path)
{
	FILE *file = fopen(path, "rb");
	size_t file_size, bytes_read;
	char *buffer;

	if (file == NULL) {
		fprintf(stderr, "Could not open file \"%s\".\n", path);
		exit(74);
	}

	fseek(file, 0L, SEEK_END);
	file_size = ftell(file);
	rewind(file);

	buffer = malloc(file_size + 1);
	if (buffer == NULL) {
		fprintf(stderr, "Not enough memory to read \"%s\".", path);
		exit(74);
	}
	bytes_read = fread(buffer, sizeof(char), file_size, file);
	if (bytes_read < file_size) {
		fprintf(stderr, "Could not read file \"%s\".", path);
		exit(74);
	}
	buffer[bytes_read] = '\0';

	fclose(file);
	return buffer;
}

static void run_file(const char *path)
{
	char *source = read_file(path);
	enum interpret_result result = interpret(source);
	free(source);

	if (result == INTERPRET_COMPILE_ERROR)
		exit(65);
	if (result == INTERPRET_RUNTIME_ERROR)
		exit(70);
}

int main(int argc, char *argv[])
{
	init_vm();

	if (argc == 1)
		repl();
	else if (argc == 2)
		run_file(argv[1]);
	else
		printf("Usage: clox [path/to/script]\n");

	free_vm();
	return 0;
}
