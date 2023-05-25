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

enum {
	TKN_CST, /* constant */
	TKN_ADR, /* address */
	TKN_ADS, /* address sufix */
	TKN_INS, /* instruction */
	TKN_ACS, /* access operator */
	TKN_LBL, /* label */
	TKN_MLB, /* most significant byte of label */
	TKN_LLB, /* least significant byte of label */
	TKN_STR, /* string */
};

enum lbltyp {
	LTP_NORMAL,
	LTP_MOST,
	LTP_LEAST,
};

enum keyword {
	KW_BYTE,
	KW_WORD,
	KW_STRDEF,
	KW_STR,
};

struct constant {
	const i8 *name;
	u32       name_siz;
	b8 is_word;
	union {
		u8  byte;
		u16 word;
	};
};

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
	struct split string;
	union {
		enum opcode opcode;
		enum lbltyp lbltyp;
	};
	union {
		u8  byte;
		u16 word;
	};
};

struct label {
	struct split  string;
	b8            is_strdef;
	b8            is_struct;
	u32           strdef_idx;
	struct label *members;
	union {
		u16 size;
		u16 addr;
		u16 offset;
	};
};

static struct constant predefined_constants[] = {
	{ .name = "spr_attr",  .name_siz = 8, .is_word = 1, .word = SPR_ATTR  },
	{ .name = "screen00",  .name_siz = 8, .is_word = 1, .word = SCREEN00  },
	{ .name = "screen01",  .name_siz = 8, .is_word = 1, .word = SCREEN01  },
	{ .name = "screen10",  .name_siz = 8, .is_word = 1, .word = SCREEN10  },
	{ .name = "screen11",  .name_siz = 8, .is_word = 1, .word = SCREEN11  },
	{ .name = "bg_attr",   .name_siz = 7, .is_word = 1, .word = BG_ATTR   },
	{ .name = "in_out",    .name_siz = 6, .is_word = 1, .word = IN_OUT    },
	{ .name = "bg_tiles",  .name_siz = 8, .is_word = 1, .word = BG_TILES  },
	{ .name = "spr_tiles", .name_siz = 9, .is_word = 1, .word = SPR_TILES },
	{ .name = "bg_pals",   .name_siz = 7, .is_word = 1, .word = BG_PALS   },
	{ .name = "spr_pals",  .name_siz = 8, .is_word = 1, .word = SPR_PALS  },
	{ .name = "scroll_x",  .name_siz = 8, .is_word = 1, .word = SCROLL_X  },
	{ .name = "scroll_y",  .name_siz = 8, .is_word = 1, .word = SCROLL_Y  },
	{ .name = "bg_tile",   .name_siz = 7, .is_word = 1, .word = BG_TILE   },
};
static u32 predefined_constants_amount = sizeof (predefined_constants) / sizeof (struct constant);

static struct split *splits;
static struct token *tokens;
static struct label *labels;
static i8           *file_name;
static const i8      separators[] = " \t\n";
static const i8      operators[]  = ":&$%@.,<>";
static i32
get_label(struct label *labels, struct split string) {
	for (u32 i = 0; i < arraylist_size(labels); i++) {
		if (string.siz == labels[i].string.siz && strncmp(string.buf, labels[i].string.buf, string.siz) == 0)
			return i;
	}
	return -1;
}

static b8
label_validate(struct label *labels, struct split label_name) {
	i32 opcode = cpu_opcode_get(label_name.buf, label_name.siz);
	if (opcode != -1) {
		fprintf(stderr, "error: %s : %u,%u: trying to define label '%.*s', but it's an instruction\n",
			 file_name, label_name.line, label_name.col, label_name.siz, label_name.buf);
		return 0;
	}
	if (get_label(labels, label_name) != -1) {
		fprintf(stderr, "error: %s : %u,%u: trying to redefine label '%.*s'\n",
			 file_name, label_name.line, label_name.col, label_name.siz, label_name.buf);
		return 0;
	}
	return 1;
}

static i32
get_keyword(struct split string) {
	switch (string.siz) {
		case 3:
			if (strncmp(string.buf, "str", string.siz) == 0) return KW_STR;
			else return -1;
		case 4:
			     if (strncmp(string.buf, "byte", string.siz) == 0) return KW_BYTE;
			else if (strncmp(string.buf, "word", string.siz) == 0) return KW_WORD;
			else return -1;
		case 6:
			if (strncmp(string.buf, "strdef", string.siz) == 0) return KW_STRDEF;
			else return -1;
		default:
			return -1;
	}
}

static b8 
split(const i8 *source) {
	splits = arraylist_alloc(sizeof (struct split));
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
						 file_name, line, col);
					return 0;
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
			splits = arraylist_push(splits, &(struct split) { source++, 1, line, col, 0 });
			split = NULL;
			col++;
			continue;
		}
assemble_split:
		if (!split) {
			splits = arraylist_push(splits, &(struct split) { source, 0, line, col, is_str });
			split = splits + (arraylist_size(splits) - 1);
		}
		split->siz++;
		col++;
		source++;
	}
	return 1;
}

static b8
lex(void) {
	tokens = arraylist_alloc(sizeof (struct token));
	labels = arraylist_alloc(sizeof (struct label));
	u32 code_ptr = ROM_BEGIN;
	u16 memory_pointer = 0x0000;
	for (u32 i = 0; i < arraylist_size(splits); i++) {
		i32 opcode;
		u32 hex, bin;
		u32 addrmd = NOA;
		i8 *invalid_char;
		if (splits[i].string_literal) {
			code_ptr += splits[i].siz;
			tokens = arraylist_push(tokens, &(struct token) { TKN_STR, .string = splits[i], .addrmd = CST });
			continue;
		}
		b8 predefined_constant = 0;
		for (u32 j = 0; j < predefined_constants_amount; j++) {
			if (splits[i].siz != predefined_constants[j].name_siz || strncmp(predefined_constants[j].name, splits[i].buf, splits[i].siz) != 0) continue;
			u8 tkn_type = TKN_CST;
			addrmd = CST;
			code_ptr++;
			if (predefined_constants[j].is_word) {
				tkn_type = TKN_ADR;
				addrmd = ZPG;
				if (predefined_constants[j].word > 0xff) {
					addrmd = ADR;
					code_ptr++;
				}
			}
			tokens = arraylist_push(tokens, &(struct token) { tkn_type, .word = predefined_constants[j].word, .addrmd = addrmd });
			predefined_constant = 1;
			break;
		}
		if (predefined_constant) continue;
		switch (splits[i].buf[0]) {
			case '%':
				if (i + 1 > arraylist_size(splits) - 1) {
					fprintf(stderr, "error: %s : %u,%u: '%%' prefix without a constant\n",
						 file_name, splits[i].line, splits[i].col);
					return 0;
				}
				i++;
				bin = strtoul(splits[i].buf, &invalid_char, 2);
				if (splits[i].buf + splits[i].siz != invalid_char) {
					fprintf(stderr, "error: %s : %u,%u: '%.*s' is not a bin number\n",
						 file_name, splits[i].line, splits[i].col, splits[i].siz, splits[i].buf);
					return 0;
				}
				if (bin > 0b11111111) {
					fprintf(stderr, "error: %s : %u,%u: constant '%.*s' is greater than 1 byte long\n",
						 file_name, splits[i].line, splits[i].col, splits[i].siz, splits[i].buf);
					return 0;
				}
				code_ptr++;
				tokens = arraylist_push(tokens, &(struct token) { TKN_CST, .byte = bin, .addrmd = CST });
				break;
			case '$':
				if (i + 1 > arraylist_size(splits) - 1) {
					fprintf(stderr, "error: %s : %u,%u: '%%' prefix without a constant\n",
						 file_name, splits[i].line, splits[i].col);
					return 0;
				}
				i++;
				hex = strtoul(splits[i].buf, &invalid_char, 16);
				if (splits[i].buf + splits[i].siz != invalid_char) {
					fprintf(stderr, "error: %s : %u,%u: '%.*s' is not a hex number\n",
						 file_name, splits[i].line, splits[i].col, splits[i].siz, splits[i].buf);
					return 0;
				}
				if (hex > 0xff) {
					fprintf(stderr, "error: %s : %u,%u: constant '%.*s' is greater than 1 byte long\n",
						 file_name, splits[i].line, splits[i].col, splits[i].siz, splits[i].buf);
					return 0;
				}
				code_ptr++;
				tokens = arraylist_push(tokens, &(struct token) { TKN_CST, .byte = hex, .addrmd = CST });
				break;
			case '&':
				if (i + 1 > arraylist_size(splits) - 1) {
					fprintf(stderr, "error: %s : %u,%u: '$' prefix without an address\n",
						 file_name, splits[i].line, splits[i].col);
					return 0;
				}
				i++;
				hex = strtoul(splits[i].buf, &invalid_char, 16);
				if (splits[i].siz == 1 && splits[i].buf[0] == '.') {
					hex = code_ptr - 1;
				} else if (splits[i].buf + splits[i].siz != invalid_char) {
					fprintf(stderr, "error: %s : %u,%u: '%.*s' is not a hex number\n",
						 file_name, splits[i].line, splits[i].col, splits[i].siz, splits[i].buf);
					return 0;
				}
				if (hex > 0xffff) {
					fprintf(stderr, "error: %s : %u,%u: address '%.*s' is greater than 2 bytes long\n",
						 file_name, splits[i].line, splits[i].col, splits[i].siz, splits[i].buf);
					return 0;
				}
				addrmd = ZPG;
				code_ptr++;
				if (hex > 0xff) {
					addrmd = ADR;
					code_ptr++;
				}
				tokens = arraylist_push(tokens, &(struct token) { TKN_ADR, .word = hex, .addrmd = addrmd });
				break;
			case ',':
				if (i == 0 || (
					tokens[arraylist_size(tokens) - 1].type != TKN_LBL &&
					tokens[arraylist_size(tokens) - 1].type != TKN_ADR)) {
					fprintf(stderr, "error: %s : %u,%u: missing address before ',' %d\n",
						 file_name, splits[i].line, splits[i].col, tokens[arraylist_size(tokens) - 1].type);
					return 0;
				}
				if (i + 1 > arraylist_size(splits) - 1) {
					fprintf(stderr, "error: %s : %u,%u: missing register after ','\n",
						 file_name, splits[i].line, splits[i].col);
					return 0;
				}
				i++;
				if (splits[i].siz != 1 || (splits[i].buf[0] != 'x' && splits[i].buf[0] != 'y')) {
					fprintf(stderr, "error: %s : %u,%u: '%.*s' is not a valid register\n",
						 file_name, splits[i].line, splits[i].col, splits[i].siz, splits[i].buf);
					return 0;
				}
				if (tokens[arraylist_size(tokens) - 1].addrmd == ADR) {
					tokens = arraylist_push(tokens, &(struct token) { TKN_ADS, .addrmd = splits[i].buf[0] == 'x' ? ADX : ADY });
				} else {
					tokens = arraylist_push(tokens, &(struct token) { TKN_ADS, .addrmd = splits[i].buf[0] == 'x' ? ZPX : ZPY });
				}
				break;
			case ':':
				if (i == 0) {
					fprintf(stderr, "error: %s : %u,%u: trying to define label without a name\n",
						 file_name, splits[i].line, splits[i].col);
					return 0;
				}
				i--;
				if (!label_validate(labels, splits[i])) return 0;
				labels = arraylist_push(labels, &(struct label) { .string = splits[i], .addr = code_ptr, .is_strdef = 0, .is_struct = 0 });
				if (i + 2 < arraylist_size(splits) && splits[i + 2].buf[0] == '@') {
					i += 2;
					if (i + 1 >= arraylist_size(splits)) {
						fprintf(stderr, "error: %s : %u,%u: '@' needs to be followed by a keyword\n",
							 file_name, splits[i].line, splits[i].col);
						return 0;
					}
					i++;
					i32 keyword = get_keyword(splits[i]);
					if (keyword == -1) {
						fprintf(stderr, "error: %s : %u,%u: '%.*s' is not a keyword\n",
							 file_name, splits[i].line, splits[i].col, splits[i].siz, splits[i].buf);
						return 0;
					}
					labels[arraylist_size(labels) - 1].addr = memory_pointer;
					i32 str;
					switch (keyword) {
						case KW_BYTE:
							memory_pointer++;
							break;
						case KW_WORD:
							memory_pointer += 2;
							break;
						case KW_STR:
							if (i + 1 >= arraylist_size(splits)) {
								fprintf(stderr, "error: %s : %u,%u: 'str' keyword needs to be followed by a struct name\n",
									 file_name, splits[i].line, splits[i].col);
								return 0;
							}
							str = get_label(labels, splits[++i]);
							if (str == -1 || !labels[str].is_strdef) {
								fprintf(stderr, "error: %s : %u,%u: '%.*s' is not a struct\n",
									 file_name, splits[i].line, splits[i].col, splits[i].siz, splits[i].buf);
								return 0;
							}
							labels[arraylist_size(labels) - 1].is_struct  = 1;
							labels[arraylist_size(labels) - 1].strdef_idx = str;
							memory_pointer += labels[str].size;
							break;
						case KW_STRDEF:
							if (i + 1 >= arraylist_size(splits)) {
								fprintf(stderr, "error: %s : %u,%u: empty struct definition\n",
									 file_name, splits[i].line, splits[i].col);
								return 0;
							}
							labels[arraylist_size(labels) - 1].is_strdef = 1;
							labels[arraylist_size(labels) - 1].members   = arraylist_alloc(sizeof (struct label));
							labels[arraylist_size(labels) - 1].size      = 0;
							for (i++; i < arraylist_size(splits); i++) {
								if (strchr(operators, splits[i].buf[0])) {
									fprintf(stderr, "error: %s : %u,%u: '%c' is not valid in a label definition\n",
										 file_name, splits[i].line, splits[i].col, splits[i].buf[0]);
									return 0;
								}
								if (!label_validate(labels[arraylist_size(labels) - 1].members, splits[i])) {
									return 0;
								}
								if (i + 1 >= arraylist_size(splits) || splits[i + 1].buf[0] != ':')  {
									fprintf(stderr, "error: %s : %u,%u: label call is not valid in a struct definition (forgot ':' ?)\n",
										 file_name, splits[i].line, splits[i].col);
									return 0;
								}
								labels[arraylist_size(labels) - 1].members = arraylist_push(
										labels[arraylist_size(labels) - 1].members,
										&(struct label) { .string = splits[i], .offset = labels[arraylist_size(labels) - 1].size, .is_strdef = 0 });
								i++;
								if (i + 2 >= arraylist_size(splits)
									|| splits[i + 1].buf[0] != '@'
									|| (get_keyword(splits[i + 2]) != KW_BYTE && get_keyword(splits[i + 2]) != KW_WORD))  {
									fprintf(stderr, "error: %s : %u,%u: only variable labels are valid inside a struct definition\n",
										 file_name, splits[i].line, splits[i].col);
									return 0;
								}
								i += 2;
								switch (get_keyword(splits[i])) {
									case KW_BYTE:
										labels[arraylist_size(labels) - 1].size++;
										break;
									case KW_WORD:
										labels[arraylist_size(labels) - 1].size += 2;
										break;
									default:
										assert(0);
										break;
								}
								if (i + 1 < arraylist_size(splits) && splits[i + 1].buf[0] != ',') break;
								else i++;
							}
							break;
						default:
							assert(0);
							break;
					}
				} else {
					i++;
				}
				break;
			case '@':
				fprintf(stderr, "error: %s : %u,%u: keywords needs to be led by a label\n",
					 file_name, splits[i].line, splits[i].col);
				return 0;
				break;
			case '.':
				if (i + 1 > arraylist_size(splits) - 1) {
					fprintf(stderr, "error: %s : %u,%u: '.' needs to be followed by a member of a struct\n",
						 file_name, splits[i].line, splits[i].col);
					return 0;
				}
				i++;
				if (cpu_opcode_get(splits[i].buf, splits[i].siz) != -1) {
					fprintf(stderr, "error: %s : %u,%u: trying to access '%.*s' from a struct, but it's a instruction\n",
						 file_name, splits[i].line, splits[i].col, splits[i].siz, splits[i].buf);
					return 0;
				}
				tokens = arraylist_push(tokens, &(struct token) { TKN_ACS, .string = splits[i] } );
				break;
			case '<':
			case '>':
				break;
			default:
				opcode = cpu_opcode_get(splits[i].buf, splits[i].siz);
				if (opcode != -1) {
					tokens = arraylist_push(tokens, &(struct token) { TKN_INS, .opcode = opcode, .addrmd = NOA});
					code_ptr++;
				} else if (i >= arraylist_size(splits) - 1 || splits[i + 1].buf[0] != ':') {
					enum lbltyp lbltyp = LTP_NORMAL;
					if (i > 0 && splits[i - 1].siz == 1) {
						lbltyp = splits[i - 1].buf[0] == '<' ? LTP_LEAST : splits[i - 1].buf[0] == '>' ? LTP_MOST : LTP_NORMAL;
					}
					code_ptr++;
					tokens = arraylist_push(tokens, &(struct token) { TKN_LBL, .string = splits[i], .addrmd = lbltyp == LTP_NORMAL ? ADR : ZPG, .lbltyp = lbltyp });
					code_ptr += lbltyp == LTP_NORMAL ;
				}
				break;
		}
	}
	return 1;
}

static b8
parse(void) {
	u32 code_ptr = ROM_BEGIN;
	u16 addr;
	i32 label;
	for (u32 i = 0; i < arraylist_size(tokens); i++) {
		switch (tokens[i].type) {
			case TKN_CST:
				if (i == 0 || tokens[i - 1].type != TKN_INS || tokens[i - 1].addrmd != CST) {
					putchar('\n');
					printf("[%.4x]", code_ptr);
				}
				bus_write_byte(code_ptr++, tokens[i].byte);
				printf(" $%.2x", tokens[i].byte);
				break;
			case TKN_ADR:
				if (i == 0 || tokens[i - 1].type != TKN_INS || tokens[i - 1].addrmd == NOA || tokens[i - 1].addrmd == CST) {
					putchar('\n');
					printf("[%.4x]", code_ptr);
				}
				if (tokens[i].addrmd == ZPG) {
					bus_write_byte(code_ptr++, tokens[i].byte);
					printf(" &%.2x", tokens[i].byte);
				} else {
					bus_write_word(code_ptr++, tokens[i].word);
					printf(" &%.4x", tokens[i].word);
				}
				break;
			case TKN_INS:
				if (i > 0) putchar('\n');
				printf("[%.4x] %s", code_ptr, cpu_opcode_str(tokens[i].opcode));
				if (i < arraylist_size(tokens) - 1) {
					if (i + 1 < arraylist_size(tokens) - 1 && tokens[i + 2].type == TKN_ADS) {
						tokens[i].addrmd = tokens[i + 2].addrmd;
					} else {
						tokens[i].addrmd = tokens[i + 1].addrmd;
					}
				}
				bus_write_byte(code_ptr, cpu_instruction_get(tokens[i].opcode, tokens[i].addrmd));
				code_ptr += 2;
				break;
			case TKN_LBL:
				if (i == 0 || tokens[i - 1].type != TKN_INS || tokens[i - 1].addrmd == NOA || tokens[i - 1].addrmd == CST) {
					putchar('\n');
					printf("[%.4x]", code_ptr);
				}
				label = get_label(labels, tokens[i].string);
				if (label == -1) {
					fprintf(stderr, "error: %s : %u,%u: label '%.*s' is not defined\n",
						 file_name, tokens[i].string.line, tokens[i].string.col, tokens[i].string.siz, tokens[i].string.buf);
					return 0;
				}
				if (labels[label].is_strdef) {
					fprintf(stderr, "error: %s : %u,%u: label '%.*s' is not an access label\n",
						 file_name, tokens[i].string.line, tokens[i].string.col, tokens[i].string.siz, tokens[i].string.buf);
					return 0;
				}
				addr = labels[label].addr;
				if (i + 1 < arraylist_size(tokens) && tokens[i + 1].type == TKN_ACS) {
					if (!labels[label].is_struct) {
						fprintf(stderr, "error: %s : %u,%u: label '%.*s' is not a struct\n",
							 file_name, tokens[i].string.line, tokens[i].string.col, tokens[i].string.siz, tokens[i].string.buf);
						return 0;
					}
					i++;
					i32 member = get_label(labels[labels[label].strdef_idx].members, tokens[i].string);
					if (member == -1) {
						fprintf(stderr, "error: %s : %u,%u: label '%.*s' is not a member of the struct\n",
							 file_name, tokens[i + 1].string.line, tokens[i + 1].string.col, tokens[i].string.siz, tokens[i].string.buf);
						return 0;
					}
					addr += labels[labels[label].strdef_idx].members[member].offset;
					i--;
				}
				if (tokens[i].lbltyp == LTP_NORMAL) {
					printf(" &%.4x", addr);
					bus_write_word(code_ptr, addr);
					code_ptr += 2;
				} else if (tokens[i].lbltyp == LTP_LEAST) {
					printf(" &%.2x", addr & 0xff);
					bus_write_byte(code_ptr++, addr & 0xff);
				} else if (tokens[i].lbltyp == LTP_MOST) {
					printf(" &%.2x", addr >> 8);
					bus_write_byte(code_ptr++, addr >> 8); 
				} else {
					assert(0);
				}
				break;
			case TKN_STR:
				printf("\n[%.4x]", code_ptr);
				for (u32 j = 0; j < tokens[i].string.siz; j++) {
					printf(" $%.2x", tokens[i].string.buf[j]);
					bus_write_byte(code_ptr++, tokens[i].string.buf[j]);
				}
				break;
			default:
				break;
		}
	}
	putchar('\n');
	return 1;
}

b8
assemble(i8 *name) {
	b8 error = 0;
	/* load file */
	file_name = name;
	i8 *path = resource_path("data", file_name);
	FILE *file = fopen(path, "r");
	if (!file) {
		fprintf(stderr, "error: couldn't open file %s: %s\n", file_name, strerror(errno));
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
	fclose(file);
	error = split(source) && lex() && parse();
	printf("%d\n", error);
	free(source);
	if (splits) arraylist_free(splits);
	if (tokens) arraylist_free(tokens);
	for (u32 i = 0; i < arraylist_size(labels); i++) {
		if (labels[i].is_strdef)
			arraylist_free(labels[i].members);
	}
	if (labels) arraylist_free(labels);
	return error;
}
