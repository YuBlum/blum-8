#include "types.h"
#include <math.h>
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

#define DISASSEMBLE 0

#define MAX(V1, V2) (V1 > V2 ? V1 : V2)

enum {
  TKN_NIL,
  TKN_CST, /* constant */
  TKN_ADR, /* address */
  TKN_ADS, /* address sufix */
  TKN_CAD, /* current address */
  TKN_INS, /* instruction */
  TKN_ACS, /* access operator */
  TKN_LBL, /* label */
  TKN_MLB, /* most significant byte of label */
  TKN_LLB, /* least significant byte of label */
  TKN_STR, /* string */
  TKN_ORG, /* origin */
  TKN_EXP, /* expression */
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

enum {
  EXP_PLS,
  EXP_MNS,
  EXP_MUL,
  EXP_CST,
  EXP_ADR,
  EXP_COUNT
};

struct expression {
  u32 type;
  u32 priority;
  u16 num;
  u32 line;
  u32 col;
  u8  symbol;
  struct expression *lhs;
  struct expression *rhs;
  struct expression *prv;
};

struct exp_constant {
  u16 num;
  u32 type;
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
  struct expression *expression;
  union {
    enum opcode opcode;
    enum lbltyp lbltyp;
    b8 has_number;
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
  u32           split_pos;
  struct label *members;
  union {
    u16 size;
    u16 addr;
    u16 offset;
  };
};

static struct constant predefined_constants[] = {
  { .name = "zero_page", .name_siz =  9, .is_word = 1, .word = ZERO_PAGE },
  { .name = "stack_end", .name_siz = 11, .is_word = 1, .word = STACK_END },
  { .name = "spr_attr",  .name_siz =  8, .is_word = 1, .word = SPR_ATTR  },
  { .name = "screen00",  .name_siz =  8, .is_word = 1, .word = SCREEN00  },
  { .name = "screen01",  .name_siz =  8, .is_word = 1, .word = SCREEN01  },
  { .name = "screen10",  .name_siz =  8, .is_word = 1, .word = SCREEN10  },
  { .name = "screen11",  .name_siz =  8, .is_word = 1, .word = SCREEN11  },
  { .name = "bg_attr",   .name_siz =  7, .is_word = 1, .word = BG_ATTR   },
  { .name = "in_out",    .name_siz =  6, .is_word = 1, .word = IN_OUT    },
  { .name = "bg_tiles",  .name_siz =  8, .is_word = 1, .word = BG_TILES  },
  { .name = "spr_tiles", .name_siz =  9, .is_word = 1, .word = SPR_TILES },
  { .name = "bg_pals",   .name_siz =  7, .is_word = 1, .word = BG_PALS   },
  { .name = "spr_pals",  .name_siz =  8, .is_word = 1, .word = SPR_PALS  },
  { .name = "scroll_x",  .name_siz =  8, .is_word = 1, .word = SCROLL_X  },
  { .name = "scroll_y",  .name_siz =  8, .is_word = 1, .word = SCROLL_Y  },
  { .name = "rom_begin", .name_siz =  9, .is_word = 1, .word = ROM_BEGIN },
  { .name = "vectors",   .name_siz =  7, .is_word = 1, .word = VECTORS   },
};
static u32 predefined_constants_amount = sizeof (predefined_constants) / sizeof (struct constant);

static u32 expressions_priorities[EXP_COUNT] = {
  [EXP_CST] = 0,
  [EXP_ADR] = 0,
  [EXP_MNS] = 1,
  [EXP_PLS] = 1,
  [EXP_MUL] = 2,
};

static struct split *splits;
static struct token *tokens;
static struct label *labels;
static i8           *file_name;
static const i8      separators[] = " \t\n";
static const i8      operators[]  = ":&$%@.,<>+-*";

static i32
get_label(struct label *labels, struct split string) {
  for (u32 i = 0; i < arraylist_size(labels); i++) {
    if (string.siz == labels[i].string.siz && strncmp(string.buf, labels[i].string.buf, string.siz) == 0)
      return i;
  }
  return -1;
}

static b8
label_validate(struct label *labels, struct split label_name, u32 position) {
  i32 opcode = cpu_opcode_get(label_name.buf, label_name.siz);
  if (opcode != -1) {
    fprintf(stderr, "error: %s : %u,%u: trying to define label '%.*s', but it's an instruction\n",
       file_name, label_name.line, label_name.col, label_name.siz, label_name.buf);
    return 0;
  }
  for (u32 i = 0; i < predefined_constants_amount; i++) {
    if (strlen(predefined_constants[i].name) == label_name.siz && strncmp(label_name.buf, predefined_constants[i].name, label_name.siz) == 0) {
      fprintf(stderr, "error: %s : %u,%u: trying to define label '%.*s', but it's an predefined constant\n",
         file_name, label_name.line, label_name.col, label_name.siz, label_name.buf);
      return 0;
    }
  }
  i32 label = get_label(labels, label_name);
  if (label != -1 && labels[label].split_pos != position) {
    fprintf(stderr, "error: %s : %u,%u: trying to redefine label '%.*s'\n",
       file_name, label_name.line, label_name.col, label_name.siz, label_name.buf);
    return 0;
  }
  if (strchr(operators, label_name.buf[0])) {
    fprintf(stderr, "error: %s : %u,%u: '%c' is not valid in a label definition\n",
       file_name, label_name.line, label_name.col, label_name.buf[0]);
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
      if      (strncmp(string.buf, "byte", string.siz) == 0) return KW_BYTE;
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
  if (!splits) return 0;
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
gen_structs(void) {
  labels = arraylist_alloc(sizeof (struct label));
  if (!labels) return 0;
  b8 in_struct_def = 0;
  for (u32 i = 0; i < arraylist_size(splits); i++) {
    u32 cur_str = 0;
    if (!in_struct_def) {
      if (
        splits[i].buf[0] != '@'                 ||
        i < 2                                   ||
        i >= arraylist_size(splits) - 1         ||
        get_keyword(splits[i + 1]) != KW_STRDEF ||
        splits[i - 1].buf[0] != ':') continue;
      if (!label_validate(labels, splits[i - 2], i)) return 0;
      if (i + 2 >= arraylist_size(splits)) {
        fprintf(stderr, "error: %s : %u,%u: empty struct definition\n",
           file_name, splits[i].line, splits[i].col);
        return 0;
      }
      labels = arraylist_push(
        labels,
        &(struct label) {
          .string    = splits[i - 2], 
          .members   = arraylist_alloc(sizeof (struct label)),
          .is_strdef = 1,
          .is_struct = 0,
          .size      = 0,
          .split_pos = i - 2
        }
      );
      cur_str = arraylist_size(labels) - 1;
      i += 2;
      in_struct_def = 1;
    }
    if (!label_validate(labels[cur_str].members, splits[i], i)) return 0;
    if (i + 1 >= arraylist_size(splits) || splits[i + 1].buf[0] != ':')  {
      fprintf(stderr, "error: %s : %u,%u: label call is not valid in a struct definition (forgot ':' ?)\n",
         file_name, splits[i].line, splits[i].col);
      return 0;
    }
    labels[cur_str].members = arraylist_push(
      labels[cur_str].members,
      &(struct label) { .string = splits[i], .offset = labels[cur_str].size, .is_strdef = 0, .split_pos = i }
    );
    i++;
    if (i + 2 >= arraylist_size(splits) || splits[i + 1].buf[0] != '@'
    || (get_keyword(splits[i + 2]) != KW_BYTE && get_keyword(splits[i + 2]) != KW_WORD)) {
      fprintf(stderr, "error: %s : %u,%u: only variable labels are valid inside a struct definition\n",
         file_name, splits[i].line, splits[i].col);
      return 0;
    }
    i += 2;
    switch (get_keyword(splits[i])) {
      case KW_BYTE:
        labels[cur_str].size++;
        break;
      case KW_WORD:
        labels[cur_str].size += 2;
        break;
      default:
        assert(0);
        break;
    }
    if (i + 1 < arraylist_size(splits) && splits[i + 1].buf[0] != ',') in_struct_def = 0;
    else i++;
  }
  return 1;
}

static b8
lex_number(u32 *index, u32 *code_ptr, u32 base, i8 prefix, b8 address) {
  u32 tokens_size = arraylist_size(tokens);
  i8 *invalid_char;
  if (*index + 1 > arraylist_size(splits) - 1) {
    fprintf(stderr, "error: %s : %u,%u: '%c' prefix without a %s\n",
    file_name, splits[*index].line, splits[*index].col, prefix, address ? "address" : "constant");
    return 0;
  }
  (*index)++;
  u32 num = strtoul(splits[*index].buf, &invalid_char, base);
  if (splits[*index].siz == 1 && splits[*index].buf[0] == '.' && address) {
    *code_ptr += 2;
    tokens = arraylist_push(tokens, &(struct token) { TKN_CAD, .addrmd = ADR, .string = splits[*index] });
    return 1;
  } else if (splits[*index].buf + splits[*index].siz != invalid_char) {
    fprintf(stderr, "error: %s : %u,%u: '%c%.*s' is not a valid %s\n",
    file_name, splits[*index].line, splits[*index].col, prefix, splits[*index].siz, splits[*index].buf, address ? "address" : "constant");
    return 0;
  }
  u32 old_code_ptr = *code_ptr;
  (*code_ptr)++;
  if (address) {
    if (num > 0xffff) {
      fprintf(stderr, "error: %s : %u,%u: address '%.*s' is greater than 2 bytes long\n",
         file_name, splits[*index].line, splits[*index].col, splits[*index].siz, splits[*index].buf);
      return 0;
    }
    u32 addrmd = ZPG;
    if (num > 0xff) {
      addrmd = ADR;
      (*code_ptr)++;
    }
    if (tokens_size > 0 && tokens[tokens_size - 1].type == TKN_ORG && !tokens[tokens_size - 1].has_number) {
      /*
      if (num < ROM_BEGIN) {
        fprintf(stderr, "error: %s : %u,%u: '@org' expects an address in the ROM area (&%.4x to &ffff)\n",
        file_name, splits[*index].line, splits[*index].col, ROM_BEGIN);
        return 0;
      }
      */
      tokens[tokens_size - 1].has_number = 1;
      tokens[tokens_size - 1].word = num;
      *code_ptr = num;
    } else if (tokens_size > 0 && tokens[tokens_size - 1].type == TKN_EXP) {
      struct expression *prv = tokens[tokens_size - 1].expression;
      while (prv->rhs) prv   = prv->rhs;
      prv->rhs               = malloc(sizeof (struct expression));
      prv->rhs->type         = EXP_ADR;
      prv->rhs->lhs          = NULL;
      prv->rhs->rhs          = NULL;
      prv->rhs->num          = num;
      prv->rhs->prv          = prv;
      prv->rhs->priority     = expressions_priorities[EXP_ADR];
      *code_ptr = old_code_ptr;
    } else {
      tokens = arraylist_push(tokens, &(struct token) { TKN_ADR, .word = num, .addrmd = addrmd, .string = splits[*index] });
    }
  } else {
    if (num > 0xff) {
      fprintf(stderr, "error: %s : %u,%u: constant '%c%.*s' is greater than 1 byte long\n",
      file_name, splits[*index].line, splits[*index].col, prefix, splits[*index].siz, splits[*index].buf);
      return 0;
    }
    if (tokens_size > 0 && tokens[tokens_size - 1].type == TKN_ORG && !tokens[tokens_size - 1].has_number) {
      fprintf(stderr, "error: %s : %u,%u: '@org' expects an address but got a constant\n",
         file_name, splits[*index].line, splits[*index].col);
      return 0;
    }
    if (tokens_size > 0 && tokens[tokens_size - 1].type == TKN_EXP) {
      struct expression *prv = tokens[tokens_size - 1].expression;
      while (prv->rhs) prv   = prv->rhs;
      prv->rhs               = malloc(sizeof (struct expression));
      prv->rhs->type         = EXP_CST;
      prv->rhs->lhs          = NULL;
      prv->rhs->rhs          = NULL;
      prv->rhs->num          = num;
      prv->rhs->prv          = prv;
      prv->rhs->priority     = expressions_priorities[EXP_CST];
      *code_ptr = old_code_ptr;
    } else {
      tokens = arraylist_push(tokens, &(struct token) { TKN_CST, .word = num, .addrmd = CST, .string = splits[*index] });
    }
  }
  return 1;
}

static b8
lex_binary_operator(u8 symbol, u32 operator, struct split split) {
  u32 tokens_size = arraylist_size(tokens);
  if (
    tokens_size == 0 ||
    (
     tokens[tokens_size - 1].type != TKN_CST &&
     tokens[tokens_size - 1].type != TKN_ADR &&
     tokens[tokens_size - 1].type != TKN_EXP
   )
  ) {
    fprintf(stderr, "error: %s : %u,%u: missing left hand side of binary operator '%c'\n",
       file_name, split.line, split.col, symbol);
    return 0;
  }
  if (tokens[tokens_size - 1].type != TKN_EXP) {
    tokens[tokens_size - 1].expression            = malloc(sizeof (struct expression));
    tokens[tokens_size - 1].expression->lhs       = malloc(sizeof (struct expression));
    tokens[tokens_size - 1].expression->lhs->lhs  = NULL;
    tokens[tokens_size - 1].expression->lhs->rhs  = NULL;
    tokens[tokens_size - 1].expression->lhs->type = tokens[tokens_size - 1].type == TKN_CST ? EXP_CST : EXP_ADR;
    tokens[tokens_size - 1].expression->lhs->num  = tokens[tokens_size - 1].word;
    tokens[tokens_size - 1].expression->priority  = expressions_priorities[operator];
    tokens[tokens_size - 1].expression->rhs       = NULL;
    tokens[tokens_size - 1].expression->prv       = NULL;
    tokens[tokens_size - 1].expression->type      = operator;
    tokens[tokens_size - 1].expression->symbol    = symbol;
    tokens[tokens_size - 1].expression->col       = split.col;
    tokens[tokens_size - 1].expression->line      = split.line;
    tokens[tokens_size - 1].type                  = TKN_EXP;
  } else {
    struct expression *lhs = tokens[tokens_size - 1].expression;
    struct expression *exp = malloc(sizeof (struct expression));
    exp->type     = operator;
    exp->symbol   = symbol;
    exp->col      = split.col;
    exp->line     = split.line;
    exp->priority = expressions_priorities[operator];
    exp->rhs      = NULL;
    if (tokens[tokens_size - 1].expression->priority >= expressions_priorities[operator]) {
      exp->lhs = lhs;
      exp->prv = lhs->prv;
      lhs->prv = exp;
      tokens[tokens_size - 1].expression = exp;
    } else {
      struct expression *prv = tokens[tokens_size - 1].expression;
      while (prv->priority < expressions_priorities[operator])  {
        if (!prv->rhs) break;
        prv = prv->rhs;
      }
      exp->prv = prv->prv;
      prv->prv = exp;
      exp->prv->rhs = exp;
      exp->lhs = prv;
    }
  }
  return 1;
}

static b8
lex(void) {
  u32 code_ptr = ROM_BEGIN;
  tokens = arraylist_alloc(sizeof (struct token));
  u16 memory_pointer = 0x0000;
  for (u32 i = 0; i < arraylist_size(splits); i++) {
    i32 opcode;
    u32 addrmd = NOA;
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
      if (predefined_constants[j].is_word) {
        tkn_type = TKN_ADR;
        addrmd = ZPG;
        code_ptr++;
        if (predefined_constants[j].word > 0xff) {
          code_ptr++;
          addrmd = ADR;
        }
      }
      if (arraylist_size(tokens) > 0 && tokens[arraylist_size(tokens) - 1].type == TKN_ORG) {
        /*
        if (predefined_constants[j].word < ROM_BEGIN) {
          fprintf(stderr, "error: %s : %u,%u: '@org' expects an address in the ROM area (&%.4x to &ffff)\n",
          file_name, splits[i].line, splits[i].col, ROM_BEGIN);
          return 0;
        }
        */
        tokens[arraylist_size(tokens) - 1].has_number = 1;
        tokens[arraylist_size(tokens) - 1].word = predefined_constants[j].word;
        if (predefined_constants[j].word < ROM_BEGIN) memory_pointer = predefined_constants[j].word;
        code_ptr = predefined_constants[j].word;
      } else {
        tokens = arraylist_push(tokens, &(struct token) { tkn_type, .word = predefined_constants[j].word, .addrmd = addrmd });
      }
      predefined_constant = 1;
      break;
    }
    if (predefined_constant) continue;
    switch (splits[i].buf[0]) {
      case '%':
        if (!lex_number(&i, &code_ptr, 2, '%', 0)) return 0;
        break;
      case '$':
        if (!lex_number(&i, &code_ptr, 16, '$', 0)) return 0;
        break;
      case '&':
        if (!lex_number(&i, &code_ptr, 16, '&', 1)) return 0;
        break;
      case '+':
        if (!lex_binary_operator('+', EXP_PLS, splits[i])) return 0;
        break;
      case '-':
        if (!lex_binary_operator('-', EXP_MNS, splits[i])) return 0;
        break;
      case '*':
        if (!lex_binary_operator('*', EXP_MUL, splits[i])) return 0;
        break;
      case ',':
        if (i == 0 || (
          tokens[arraylist_size(tokens) - 1].type != TKN_LBL &&
          tokens[arraylist_size(tokens) - 1].type != TKN_ADR)) {
          fprintf(stderr, "error: %s : %u,%u: missing address before ','\n",
             file_name, splits[i].line, splits[i].col);
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
        if (!label_validate(labels, splits[i], i)) return 0;
        if (i + 2 < arraylist_size(splits) && splits[i + 2].buf[0] == '@') {
          labels = arraylist_push(
              labels, &(struct label) { .string = splits[i], .addr = memory_pointer, .is_strdef = 0, .is_struct = 0, .split_pos = i });
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
              arraylist_pop(labels, NULL);
              for (i += 5; i < arraylist_size(splits); i += 5) {
                if (splits[i].buf[0] != ',') break;
              }
              i--;
              break;
            default:
              assert(0);
              break;
          }
        } else {
          labels = arraylist_push(
              labels, &(struct label) { .string = splits[i], .addr = code_ptr, .is_strdef = 0, .is_struct = 0, .split_pos = i });
          i++;
        }
        break;
      case '@':
        if (i + 1 >= arraylist_size(splits) - 1) {
          fprintf(stderr, "error: %s : %u,%u: '@' without a keyword\n",
             file_name, splits[i].line, splits[i].col);
          return 0;
        }
        i++;
        if ((splits[i].siz == 4 && strncmp(splits[i].buf, "byte",   4) == 0)
         || (splits[i].siz == 4 && strncmp(splits[i].buf, "word",   4) == 0)
         || (splits[i].siz == 3 && strncmp(splits[i].buf, "str",    3) == 0)
         || (splits[i].siz == 6 && strncmp(splits[i].buf, "strdef", 6) == 0)) {
          fprintf(stderr, "error: %s : %u,%u: '@%.*s' needs to be led by a label\n",
             file_name, splits[i].line, splits[i].col, splits[i].siz, splits[i].buf);
          return 0;
        }
        if (splits[i].siz != 3 || strncmp(splits[i].buf, "org", 3) != 0) {
          fprintf(stderr, "error: %s : %u,%u: unknwon keyword '@%.*s'\n",
             file_name, splits[i].line, splits[i].col, splits[i].siz, splits[i].buf);
          return 0;
        } else {
          tokens = arraylist_push(tokens, &(struct token) {
            TKN_ORG, .string = splits[i], .has_number = 0
          });
        }
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
          code_ptr++;
          tokens = arraylist_push(tokens, &(struct token) { TKN_INS, .opcode = opcode, .addrmd = NOA, .string = splits[i] });
        } else if (i >= arraylist_size(splits) - 1 || splits[i + 1].buf[0] != ':') {
          enum lbltyp lbltyp = LTP_NORMAL;
          if (i > 0 && splits[i - 1].siz == 1) {
            lbltyp = splits[i - 1].buf[0] == '<' ? LTP_LEAST : splits[i - 1].buf[0] == '>' ? LTP_MOST : LTP_NORMAL;
          }
          code_ptr += 1 + (lbltyp == LTP_NORMAL);
          tokens = arraylist_push(tokens, &(struct token) {
            TKN_LBL, .string = splits[i], .addrmd = lbltyp == LTP_NORMAL ? ADR : CST, .lbltyp = lbltyp
          });
        }
        break;
    }
  }
  return 1;
}

static struct exp_constant
parse_expression(struct expression *exp) {
  if (!exp) return (struct exp_constant) { .type = TKN_NIL };
  struct exp_constant lhs, rhs, result;
  lhs = parse_expression(exp->lhs);
  rhs = parse_expression(exp->rhs);
  switch (exp->type) {
    case EXP_PLS:
      result.num = lhs.num + rhs.num;
      result.type = lhs.type == TKN_CST && rhs.type == TKN_CST ? TKN_CST : TKN_ADR;
      if (!exp->rhs) {
        fprintf(stderr, "error: %s : %u,%u: missing right hand side of binary operator '%c'\n",
           file_name, exp->line, exp->col, exp->symbol);
        return (struct exp_constant) { .type = TKN_NIL };
      }
      break;
    case EXP_MNS:
      result.num = lhs.num - rhs.num;
      result.type = lhs.type == TKN_CST && rhs.type == TKN_CST ? TKN_CST : TKN_ADR;
      if (!exp->rhs) {
        fprintf(stderr, "error: %s : %u,%u: missing right hand side of binary operator '%c'\n",
           file_name, exp->line, exp->col, exp->symbol);
        return (struct exp_constant) { .type = TKN_NIL };
      }
      break;
    case EXP_MUL:
      result.num = lhs.num * rhs.num;
      result.type = lhs.type == TKN_CST && rhs.type == TKN_CST ? TKN_CST : TKN_ADR;
      if (!exp->rhs) {
        fprintf(stderr, "error: %s : %u,%u: missing right hand side of binary operator '%c'\n",
           file_name, exp->line, exp->col, exp->symbol);
        return (struct exp_constant) { .type = TKN_NIL };
      }
      break;
    case EXP_ADR:
      result.num = exp->num;
      result.type = TKN_ADR;
      break;
    case EXP_CST:
      result.num = exp->num;
      result.type = TKN_CST;
      break;
    default:
      assert(0);
      break;
  }
  return result;
}

static b8
expressions(void) {
  for (u32 i = 0; i < arraylist_size(tokens); i++) {
    if (tokens[i].type != TKN_EXP) continue;
    struct exp_constant result = parse_expression(tokens[i].expression);
    tokens[i].type = result.type;
    switch (result.type) {
      case TKN_CST:
        tokens[i].byte = result.num % 0x100;
        tokens[i].addrmd = CST;
        break;
      case TKN_ADR:
        tokens[i].word = result.num % 0x10000;
        tokens[i].addrmd = tokens[i].word > 0xff ? ADR : ZPG;
        break;
      case TKN_NIL:
        return 0;
        break;
      default:
        assert(0);
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
      case TKN_ORG:
        if (!tokens[i].has_number) {
          fprintf(stderr, "error: %s : %u,%u: '@org' without an address\n",
             file_name, tokens[i].string.line, tokens[i].string.col);
          return 0;
        }
        code_ptr = tokens[i].word;
        break;
      case TKN_CST:
#if DISASSEMBLE
        if (i == 0 || tokens[i - 1].type != TKN_INS || tokens[i - 1].addrmd != CST) {
          putchar('\n');
          printf("[%.4x]", code_ptr);
        }
        printf(" $%.2x", tokens[i].byte);
#endif
        if (code_ptr < ROM_BEGIN) {
          fprintf(stderr, "error: %s : %u,%u: trying to write code out of the ROM area (0x8000 to 0xffff)\n",
             file_name, tokens[i].string.line, tokens[i].string.col);
          return 0;
        }
        bus_write_byte(code_ptr++, tokens[i].byte, 0); break;
      case TKN_ADR:
#if DISASSEMBLE
        if (i == 0 || tokens[i - 1].type != TKN_INS || tokens[i - 1].addrmd == NOA || tokens[i - 1].addrmd == CST) {
          putchar('\n');
          printf("[%.4x]", code_ptr);
        }
#endif
        if (code_ptr < ROM_BEGIN) {
          fprintf(stderr, "error: %s : %u,%u: trying to write code out of the ROM area (0x8000 to 0xffff)\n",
             file_name, tokens[i].string.line, tokens[i].string.col);
          return 0;
        }
        if (tokens[i].addrmd == ZPG) {
          bus_write_byte(code_ptr++, tokens[i].byte, 0);
#if DISASSEMBLE
          printf(" &%.2x", tokens[i].byte);
#endif
        } else {
          bus_write_word(code_ptr, tokens[i].word, 0);
#if DISASSEMBLE
          printf(" &%.4x", tokens[i].word);
#endif
          code_ptr += 2;
        }
        break;
      case TKN_CAD:
#if DISASSEMBLE
        if (i == 0 || tokens[i - 1].type != TKN_INS || tokens[i - 1].addrmd == NOA || tokens[i - 1].addrmd == CST) {
          putchar('\n');
          printf("[%.4x]", code_ptr - 1);
        }
        printf(" &%.4x", code_ptr - 1);
#endif
        if (code_ptr < ROM_BEGIN) {
          fprintf(stderr, "error: %s : %u,%u: trying to write code out of the ROM area (0x8000 to 0xffff)\n",
             file_name, tokens[i].string.line, tokens[i].string.col);
          return 0;
        }
        bus_write_word(code_ptr, code_ptr - 1, 0);
        code_ptr += 2;
        break;
      case TKN_INS:
#if DISASSEMBLE
        if (i > 0) putchar('\n');
        printf("[%.4x] %s", code_ptr, cpu_opcode_str(tokens[i].opcode));
#endif
        if (code_ptr < ROM_BEGIN) {
          fprintf(stderr, "error: %s : %u,%u: trying to write code out of the ROM area (0x8000 to 0xffff)\n",
             file_name, tokens[i].string.line, tokens[i].string.col);
          return 0;
        }
        if (i < arraylist_size(tokens) - 1) {
          if (i + 1 < arraylist_size(tokens) - 1 && tokens[i + 2].type == TKN_ADS) {
            tokens[i].addrmd = tokens[i + 2].addrmd;
          } else {
            tokens[i].addrmd = tokens[i + 1].addrmd;
          }
        }
        i8 code = cpu_instruction_get(tokens[i].opcode, tokens[i].addrmd);
        if (code == -1) {
          tokens[i].addrmd = NOA;
          code = cpu_instruction_get(tokens[i].opcode, tokens[i].addrmd);
          assert(code != -1);
        }
        bus_write_byte(code_ptr++, code, 0);
        break;
      case TKN_LBL:
#if DISASSEMBLE
        if (i == 0 || tokens[i - 1].type != TKN_INS || tokens[i - 1].addrmd == NOA ||
          (tokens[i - 1].addrmd == CST && tokens[i].lbltyp == LTP_NORMAL) ||
          (tokens[i - 1].addrmd != CST && tokens[i].lbltyp != LTP_NORMAL)) {
          putchar('\n');
          printf("[%.4x]", code_ptr);
        }
#endif
        if (code_ptr < ROM_BEGIN) {
          fprintf(stderr, "error: %s : %u,%u: trying to write code out of the ROM area (0x8000 to 0xffff)\n",
             file_name, tokens[i].string.line, tokens[i].string.col);
          return 0;
        }
        label = get_label(labels, tokens[i].string);
        if (label == -1) {
          fprintf(stderr, "error: %s : %u,%u: label '%.*s' is not defined\n",
             file_name, tokens[i].string.line, tokens[i].string.col, tokens[i].string.siz, tokens[i].string.buf);
          return 0;
        }
        if (labels[label].is_strdef) {
          fprintf(stderr, "error: %s : %u,%u: label '%.*s' is a struct type\n",
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
#if DISASSEMBLE
          printf(" &%.4x", addr);
#endif
          bus_write_word(code_ptr, addr, 0);
          code_ptr += 2;
        } else if (tokens[i].lbltyp == LTP_LEAST) {
#if DISASSEMBLE
          printf(" $%.2x", addr & 0xff);
#endif
          bus_write_byte(code_ptr++, addr & 0xff, 0);
        } else if (tokens[i].lbltyp == LTP_MOST) {
#if DISASSEMBLE
          printf(" $%.2x", addr >> 8);
#endif
          bus_write_byte(code_ptr++, addr >> 8, 0); 
        } else {
          assert(0);
        }
        break;
      case TKN_STR:
#if DISASSEMBLE
        printf("\n[%.4x]", code_ptr);
#endif
        if (code_ptr < ROM_BEGIN) {
          fprintf(stderr, "error: %s : %u,%u: trying to write code out of the ROM area (0x8000 to 0xffff)\n",
             file_name, tokens[i].string.line, tokens[i].string.col);
          return 0;
        }
        for (u32 j = 0; j < tokens[i].string.siz; j++) {
#if DISASSEMBLE
          printf(" $%.2x", tokens[i].string.buf[j]);
#endif
          bus_write_byte(code_ptr++, tokens[i].string.buf[j], 0);
        }
        break;
      default:
        break;
    }
  }
#if DISASSEMBLE
  putchar('\n');
#endif
  return 1;
}

b8
assemble(i8 *name) {
  assert(predefined_constants_amount == MAX_PREDEF);
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
  error = split(source) && gen_structs() && lex() && expressions() && parse();
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
