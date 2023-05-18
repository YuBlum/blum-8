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

struct token {
	u32 type;
	union {
		u8  byte;
		u16 word;
		struct {
			i8  name[256];
			u16 lbl_word;
		};
		struct {
			enum opcode opcode;
			enum addrmd addrmd;
			union {
				u8  inst_byte;
				u16 inst_word;
			};
		};
	};
};

static struct token *tokens;

static u32
get_hex(i8 **source, i8 *token, i8 **invalid_char) {
	u32 siz = 0;
	while ((**source >= 'a' && **source <= 'z') || (**source >= 'A' && **source <= 'Z') || (**source >= '0' && **source <= '9')) {
		token[siz++] = **source;
		(*source)++;
	}
	token[siz] = '\0';
	return strtoul(token, invalid_char, 16);
}

void
assembler_lex(const i8 *input) {
	/* load file */
	i8 *path = resource_path("data", input);
	FILE *file = fopen(path, "r");
	if (!file) {
		fprintf(stderr, "error: couldn't open file %s: %s\n", input, strerror(errno));
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
	/* lexify */
	tokens = arraylist_alloc(sizeof (struct token));
	i8 *original_source_ptr = source;
	i64 cur_inst = -1;
	while (*source) {
		i8  token[256];
		if (*source == '%') {
			i8 *invalid_char;
			++source;
			u32 hex = get_hex(&source, token, &invalid_char);
			if (*invalid_char != '\0') {
				fprintf(stderr, "error: assembler: '%%%s' is not a valid hex number\n", token);
				exit(1);
			}
			if (hex > 0xff) {
				fprintf(stderr, "error: assembler: '%%%s' hex constants needs to be 1 byte long\n", token);
				exit(1);
			}
			if (cur_inst != -1) {
				tokens[cur_inst].addrmd    = CST;
				tokens[cur_inst].inst_byte = hex;
				cur_inst = -1;
			} else {
				tokens = arraylist_push(tokens, &(struct token) { TKN_CST, { .byte = hex } });
			}
		} else if (*source == '$') {
			i8 *invalid_char;
			++source;
			u32 hex = get_hex(&source, token, &invalid_char);
			if (*invalid_char != '\0') {
				fprintf(stderr, "error: assembler: '$%s' is not a valid address\n", token);
				exit(1);
			}
			if (hex > 0xffff) {
				fprintf(stderr, "error: assembler: '$%s' addresses needs to be 2 bytes long\n", token);
				exit(1);
			}
			if (cur_inst != -1) {
				tokens[cur_inst].addrmd = hex > 0xff ? ADR : ZPG;
				tokens[cur_inst].inst_word = hex;
				cur_inst = -1;
			} else {
				if (hex <= 0xff) {
					tokens = arraylist_push(tokens, &(struct token) { TKN_ZPG, { .byte = hex } });
				} else {
					tokens = arraylist_push(tokens, &(struct token) { TKN_ADR, { .word = hex } });
				}
			}
		} else if (*source >= 'a' && *source <= 'z') {
			u32 token_size = 0;
			for (; *source >= 'a' && *source <= 'z'; source++) {
				token[token_size++] = *source;
			}
			token[token_size] = '\0';
			cur_inst = -1;
			i32 opcode = cpu_opcode_get(token, token_size);
			if (opcode != -1) {
				tokens = arraylist_push(tokens, &(struct token) { TKN_INS, { .opcode = opcode, .addrmd = NOA } });
				cur_inst = arraylist_size(tokens) - 1;
			}
			if (cur_inst == -1) {
				fprintf(stderr, "error: '%s' is not a valid label\n", token);
				exit(1);
			}
		} else if (*source == ';') {
			while (*source != '\n') source++;
		}
		source++;
	}
	free(original_source_ptr);
}

void
assembler_parse(void) {
	/* parse tokens into machine code */
	u32 code_size = 0;
	for (u32 i = 0; i < arraylist_size(tokens); i++) {
		switch (tokens[i].type) {
			case TKN_CST:
			case TKN_ZPG:
				code_size++;
				break;
			case TKN_ADR:
				code_size += 2;
				break;
			case TKN_INS:
				code_size++;
				if (tokens[i].addrmd == CST || tokens[i].addrmd == ZPG) code_size++;
				else if (tokens[i].addrmd == ADR) code_size += 2;
				break;
			default:
				assert(0);
				break;
		}
	}
	u8 *code = malloc(code_size);
	u32 idx = 0;
	for (u32 i = 0; i < arraylist_size(tokens); i++) {
		switch (tokens[i].type) {
			case TKN_CST:
				code[idx++] = tokens[i].byte;
				break;
			case TKN_ZPG:
				code[idx++] = tokens[i].byte;
				code[idx++] = 0x00;
				break;
			case TKN_ADR:
				code[idx++] = tokens[i].word & 0xff;
				code[idx++] = tokens[i].word >> 8;
				break;
			case TKN_INS:
				code[idx++] = cpu_instruction_get(tokens[i].opcode, tokens[i].addrmd);
				if (tokens[i].addrmd == CST || tokens[i].addrmd == ZPG) {
					code[idx++] = tokens[i].inst_byte;
				} else if (tokens[i].addrmd == ADR) {
					code[idx++] = tokens[i].inst_word & 0xff;
					code[idx++] = tokens[i].inst_word >> 8;
				}
				break;
			default:
				assert(0);
				break;
		}
	}
	bus_cartridge_load(code, code_size);
	free(code);
	arraylist_free(tokens);
}
