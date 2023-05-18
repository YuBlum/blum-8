#include <os.h>
#include <cpu.h>
#include <bus.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <arraylist.h>
#include <assembler.h>

#define MAX(V1, V2) (V1 > V2 ? V1 : V2)

struct string {
	const i8 *buf;
	u32 siz;
	u32 line;
	u32 col;
};

struct token {
	u32 type;
	enum addrmd addrmd;
	union {
		enum   opcode opcode;
		struct string string;
	};
	union {
		u8  byte;
		u16 word;
	};
};

struct label {
	struct string string;
	u16           addr;
};

static struct label *labels;
static i8            separators[] = " \t\n";
static i8            operators[]  = ":$%.";

i32 get_label(struct string string) {
	for (u32 i = 0; i < arraylist_size(labels); i++) {
		if (string.siz == labels[i].string.siz && strncmp(string.buf, labels[i].string.buf, string.siz) == 0)
			return i;
	}
	return -1;
}

b8
assemble(const i8 *name) {
	b8 error = 0;
	/* load file */
	i8 *path = resource_path("data", name);
	FILE *file = fopen(path, "r");
	if (!file) {
		fprintf(stderr, "error: couldn't open file %s: %s\n", name, strerror(errno));
		return 1;
	}
	fseek(file, 0, SEEK_END);
	u32 file_size = ftell(file);
	i8 *source = malloc(file_size + 1);
	if (!source) {
		fprintf(stderr, "error: couldn't allocate memory for the file source\n");
		fclose(file);
		return 1;
	}
	rewind(file);
	fread(source, 1, file_size, file);
	source[file_size] = '\0';
	i8 *original_source_ptr = source;
	/* split source */
	struct string *splitted = arraylist_alloc(sizeof (struct string));
	struct string *split = NULL;
	u32 line = 1;
	u32 col  = 1;
	while (*source) {
		if (strchr(separators, *source)) {
			split = NULL;
			col++;
			if (*source == '\n') {
				line++;
				col = 1;
			}
			source++;
			continue;
		};
		if (*source == ';') {
			while (*source != '\n') {
				col++;
				source++;
			}
			continue;
		}
		if (strchr(operators, *source)) {
			splitted = arraylist_push(splitted, &(struct string) { source++, 1, line, col });
			continue;
		}
		if (!split) {
			splitted = arraylist_push(splitted, &(struct string) { source, 0, line, col  });
			split = splitted + (arraylist_size(splitted) - 1);
		}
		split->siz++;
		col++;
		source++;
	}
	source = original_source_ptr;
	/* lexify */
	struct token *tokens = arraylist_alloc(sizeof (struct token));
	labels = arraylist_alloc(sizeof (struct label));
	u32 code_size = 0;
	for (u32 i = 0; i < arraylist_size(splitted); i++) {
		i32 opcode;
		u32 hex;
		i8 *invalid_char;
		switch (splitted[i].buf[0]) {
			case '%':
				if (i + 1 >= arraylist_size(splitted) - 1) {
					fprintf(stderr, "error: %s : %u,%u: '%%' prefix without a constant\n",
						 name, splitted[i].line, splitted[i].col);
					error = 1;
					goto assemble_end;
				}
				i++;
				hex = strtoul(splitted[i].buf, &invalid_char, 16);
				if (splitted[i].buf + splitted[i].siz != invalid_char) {
					fprintf(stderr, "error: %s : %u,%u: '%.*s' is not a hex number\n",
						 name, splitted[i].line, splitted[i].col, splitted[i].siz, splitted[i].buf);
					error = 1;
					goto assemble_end;
				}
				if (hex > 0xff) {
					fprintf(stderr, "error: %s : %u,%u: constant '%.*s' is greater than  1 byte long\n",
						 name, splitted[i].line, splitted[i].col, splitted[i].siz, splitted[i].buf);
					error = 1;
					goto assemble_end;
				}
				code_size++;
				tokens = arraylist_push(tokens, &(struct token) { TKN_CST, .byte = hex, .addrmd = CST });
				break;
			case '$':
				if (i + 1 >= arraylist_size(splitted) - 1) {
					fprintf(stderr, "error: %s : %u,%u: '$' prefix without an address\n",
						 name, splitted[i].line, splitted[i].col);
					error = 1;
					goto assemble_end;
				}
				i++;
				hex = strtoul(splitted[i].buf, &invalid_char, 16);
				if (splitted[i].siz == 1 && splitted[i].buf[0] == '.') {
					hex = code_size - 1 + ROM_BEGIN;
				} else if (splitted[i].buf + splitted[i].siz != invalid_char) {
					fprintf(stderr, "error: %s : %u,%u: '%.*s' is not a hex number\n",
						 name, splitted[i].line, splitted[i].col, splitted[i].siz, splitted[i].buf);
					error = 1;
					goto assemble_end;
				}
				if (hex > 0xffff) {
					fprintf(stderr, "error: %s : %u,%u: address '%.*s' is greater than 2 bytes long\n",
						 name, splitted[i].line, splitted[i].col, splitted[i].siz, splitted[i].buf);
					error = 1;
					goto assemble_end;
				}
				if (hex > 0xff) {
					code_size += 2;
					tokens = arraylist_push(tokens, &(struct token) { TKN_ADR, .word = hex, .addrmd = ADR });
				} else {
					code_size++;
					tokens = arraylist_push(tokens, &(struct token) { TKN_ADR, .byte = hex, .addrmd = ZPG });
				}
				break;
			case ':':
				if (i == 0) {
					if (opcode != -1) {
						fprintf(stderr, "error: %s : %u,%u: trying to define label without a name\n",
							 name, splitted[i].line, splitted[i].col);
						error = 1;
						goto assemble_end;
					}
				}
				i--;
				opcode = cpu_opcode_get(splitted[i].buf, splitted[i].siz);
				if (opcode != -1) {
					fprintf(stderr, "error: %s : %u,%u: trying to define label '%.*s', but it's an instruction\n",
						 name, splitted[i].line, splitted[i].col, splitted[i].siz, splitted[i].buf);
					error = 1;
					goto assemble_end;
				}
				if (get_label(splitted[i]) != -1) {
					fprintf(stderr, "error: %s : %u,%u: trying to redefine label '%.*s'\n",
						 name, splitted[i].line, splitted[i].col, splitted[i].siz, splitted[i].buf);
					error = 1;
					goto assemble_end;
				}
				labels = arraylist_push(labels, &(struct label) { .string = splitted[i], .addr = code_size + ROM_BEGIN });
				i++;
				break;
			default:
				opcode = cpu_opcode_get(splitted[i].buf, splitted[i].siz);
				if (opcode != -1) {
					tokens = arraylist_push(tokens, &(struct token) { TKN_INS, .opcode = opcode, .addrmd = NOA});
					code_size++;
				} else if (i >= arraylist_size(splitted) - 1 || splitted[i + 1].buf[0] != ':') {
					tokens = arraylist_push(tokens, &(struct token) { TKN_LBL, .string = splitted[i], .addrmd = ADR });
					code_size += 2;
				}
				break;
		}
	}
	/* parse tokens */
	u8 *code = malloc(code_size);
	u32 idx = 0;
	i32 label;
	for (u32 i = 0; i < arraylist_size(tokens); i++) {
		switch (tokens[i].type) {
			case TKN_CST:
				code[idx++] = tokens[i].byte;
				break;
			case TKN_ADR:
				if (tokens[i].addrmd == ZPG) {
					code[idx++] = tokens[i].byte;
				} else {
					code[idx++] = tokens[i].word & 0xff;
					code[idx++] = tokens[i].word >> 8;
				}
				break;
			case TKN_INS:
				if (i < arraylist_size(tokens) - 1) {
					tokens[i].addrmd = tokens[i + 1].addrmd;
				}
				code[idx++] = cpu_instruction_get(tokens[i].opcode, tokens[i].addrmd);
				break;
			case TKN_LBL:
				label = get_label(tokens[i].string);
				if (label == -1) {
					fprintf(stderr, "error: %s : %u,%u: label '%.*s' is not defined\n",
						 name, tokens[i].string.line, tokens[i].string.col, tokens[i].string.siz, tokens[i].string.buf);
					error = 1;
					free(code);
					goto assemble_end;
				}
				code[idx++] = labels[label].addr & 0xff;
				code[idx++] = labels[label].addr >> 8;
				break;
			default:
				break;
		}
	}
	/* store code into memory */
	bus_cartridge_load(code, code_size);
	/* free data */
	free(code);
assemble_end:
	free(original_source_ptr);
	arraylist_free(splitted);
	arraylist_free(tokens);
	arraylist_free(labels);
	fclose(file);
	return error;
}
