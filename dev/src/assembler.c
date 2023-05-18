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

struct split {
	const i8 *buf;
	u32 siz;
	u32 line;
	u32 col;
	b8  string_literal;
};

struct token {
	u32 type;
	enum addrmd addrmd;
	union {
		enum   opcode opcode;
		struct split string;
	};
	union {
		u8  byte;
		u16 word;
	};
};

struct label {
	struct split string;
	u16           addr;
};

static struct label *labels;

i32 get_label(struct split string) {
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
	/* lists allocations */
	struct split *splitted = arraylist_alloc(sizeof (struct split));
	struct token *tokens = arraylist_alloc(sizeof (struct token));
	labels = arraylist_alloc(sizeof (struct label));
	/* split source */
	static i8 separators[] = " \t\n";
	static i8 operators[]  = ":$%.,";
	struct split *split = NULL;
	b8  is_str = 0;
	u32 line   = 1;
	u32 col    = 1;
	while (*source) {
		if (strchr(separators, *source)) {
			if (is_str) {
				if (*source == ' ' || *source == '\t') {
					goto assemble_split;
				} else {
					fprintf(stderr, "error: %s : %u,%u: missing closing '\"' on string literal\n",
						 name, line, col);
					error = 1;
					goto assemble_end;
				}
			}
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
		if (*source == '"') {
			is_str = !is_str;
			split = NULL;
			col++;
			source++;
			continue;
		}
		if (strchr(operators, *source) && !is_str) {
			splitted = arraylist_push(splitted, &(struct split) { source++, 1, line, col, 0 });
			split = NULL;
			col++;
			continue;
		}
assemble_split:
		if (!split) {
			splitted = arraylist_push(splitted, &(struct split) { source, 0, line, col, is_str });
			split = splitted + (arraylist_size(splitted) - 1);
		}
		split->siz++;
		col++;
		source++;
	}
	/* lexify */
	u32 code_size = 0;
	for (u32 i = 0; i < arraylist_size(splitted); i++) {
		i32 opcode;
		u32 hex;
		u32 addrmd = NOA;
		i8 *invalid_char;
		if (splitted[i].string_literal) {
			code_size += splitted[i].siz;
			tokens = arraylist_push(tokens, &(struct token) { TKN_STR, .string = splitted[i], .addrmd = CST });
			continue;
		}
		switch (splitted[i].buf[0]) {
			case '%':
				if (i + 1 > arraylist_size(splitted) - 1) {
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
				if (i + 1 > arraylist_size(splitted) - 1) {
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
					addrmd = ADR;
					code_size += 2;
				} else {
					addrmd = ZPG;
					code_size++;
				}
				tokens = arraylist_push(tokens, &(struct token) { TKN_ADR, .word = hex, .addrmd = addrmd });
				break;
			case ',':
				if (i == 0 || (
					tokens[arraylist_size(tokens) - 1].type != TKN_LBL &&
					tokens[arraylist_size(tokens) - 1].type != TKN_ADR)) {
					fprintf(stderr, "error: %s : %u,%u: missing address before ',' %d\n",
						 name, splitted[i].line, splitted[i].col, tokens[arraylist_size(tokens) - 1].type);
					error = 1;
					goto assemble_end;
				}
				if (i + 1 > arraylist_size(splitted) - 1) {
					fprintf(stderr, "error: %s : %u,%u: missing register after ','\n",
						 name, splitted[i].line, splitted[i].col);
					error = 1;
					goto assemble_end;
				}
				i++;
				if (splitted[i].siz != 1 || (splitted[i].buf[0] != 'x' && splitted[i].buf[0] != 'y')) {
					fprintf(stderr, "error: %s : %u,%u: '%.*s' is not a valid register\n",
						 name, splitted[i].line, splitted[i].col, splitted[i].siz, splitted[i].buf);
					error = 1;
					goto assemble_end;
				}
				if (tokens[arraylist_size(tokens) - 1].addrmd == ADR) {
					tokens = arraylist_push(tokens, &(struct token) { TKN_ADS, .addrmd = splitted[i].buf[0] == 'x' ? ADX : ADY });
				} else {
					tokens = arraylist_push(tokens, &(struct token) { TKN_ADS, .addrmd = splitted[i].buf[0] == 'x' ? ZPX : ZPY });
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
					if (i + 1 < arraylist_size(tokens) - 1 && tokens[i + 2].type == TKN_ADS) {
						tokens[i].addrmd = tokens[i + 2].addrmd;
					} else {
						tokens[i].addrmd = tokens[i + 1].addrmd;
					}
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
			case TKN_STR:
				for (u32 j = 0; j < tokens[i].string.siz; j++) {
					code[idx++] = tokens[i].string.buf[j];
				}
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
