#include <os.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assembler.h>

void
assembler_lex(const i8 *name) {
	/* load file */
	i8 *path = resource_path("data", name);
	FILE *file = fopen(path, "r");
	if (!file) {
		fprintf(stderr, "error: couldn't open file: %s\n", strerror(errno));
		exit(1);
	}
	fseek(file, 0, SEEK_END);
	u32 file_size = ftell(file);
	i8 *source = malloc(file_size + 1);
	if (!source) {
		fprintf(stderr, "error: couldn't allocate memory for the file source\n");
		exit(1);
	}
	rewind(file);
	fread(source, 1, file_size, file);
	source[file_size] = '\0';
	free(source);
	/* lexify */
}
