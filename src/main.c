#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#define IMM_FLAG    0x80
#define NOOP_LABEL  0xFF

typedef enum {
    OP_NOP      = 0x00,
    OP_ADD      = 0x01,
    OP_SUB      = 0x02,
    OP_LOAD     = 0x03,
    OP_STORE    = 0x04,
    OP_XOR      = 0x05,
    OP_AND      = 0x06,
    OP_OR       = 0x07,
    OP_NOT      = 0x08,
    OP_CMP      = 0x09,
    OP_JMP      = 0x0A,
    OP_JEQ      = 0x0B,
    OP_MOV      = 0x0C,
    OP_PRINT    = 0x0D,
} op_code;

typedef enum {
    REG_NONE    = 0x00,
    REG_A       = 0x10,
    REG_B       = 0x20,
    REG_C       = 0x30,
    REG_D       = 0x40,
    REG_E       = 0x50
} regs;

typedef struct {
    uint8_t operation;
    uint8_t argument;
} instruction_t;

typedef enum {
    INVALID_TYPE,
    NI_TYPE,
    SNI_TYPE
} filetype_t;

typedef struct {
    const char *data;
    size_t len;
} StringView;

static inline bool sv_equals(StringView sv, const char *lit) {
    size_t lit_len = strlen(lit);
    return (sv.len == lit_len) && (memcmp(sv.data, lit, lit_len) == 0);
}

typedef enum {
    TOK_EOF,
    TOK_OPCODE,
    TOK_LABEL_KW,
    TOK_REG,
    TOK_IMM,
    TOK_IDENTIFIER
} token_type_t;

typedef struct {
    token_type_t type;
    StringView text;
    uint8_t value;
    uint32_t line;
    uint32_t col;
} token_t;

typedef struct {
    const char *src;
    size_t len;
    size_t cursor;
    uint32_t line;
    uint32_t col;
} lexer_t;

static lexer_t lexer_init(const char *src, size_t len) {
    return (lexer_t){
        .src = src,
        .len = len,
        .cursor = 0,
        .line = 1,
        .col = 1
    };
}

static void lexer_skip_whitespace_and_comments(lexer_t *l) {
    while (l->cursor < l->len) {
        char c = l->src[l->cursor];
        if (c == ' ' || c == '\t' || c == '\r' || c == ',') {
            l->cursor++;
            l->col++;
        } else if (c == '\n') {
            l->cursor++;
            l->line++;
            l->col = 1;
        } else if (c == ';' || c == '#') {
            while (l->cursor < l->len && l->src[l->cursor] != '\n') {
                l->cursor++;
            }
        } else {
            break;
        }
    }
}

static token_t lexer_next(lexer_t *l) {
    lexer_skip_whitespace_and_comments(l);

    if (l->cursor >= l->len) {
        return (token_t){ .type = TOK_EOF, .line = l->line, .col = l->col };
    }

    const char *start_ptr = &l->src[l->cursor];
    uint32_t start_col = l->col;
    uint32_t start_line = l->line;
    size_t start_cursor = l->cursor;

    if (isdigit((unsigned char)l->src[l->cursor])) {
        while (l->cursor < l->len && (isxdigit((unsigned char)l->src[l->cursor]) || l->src[l->cursor] == 'x' || l->src[l->cursor] == 'X')) {
            l->cursor++;
            l->col++;
        }
        StringView sv = { .data = start_ptr, .len = l->cursor - start_cursor };
        uint8_t imm_val = (uint8_t)strtoul(sv.data, NULL, 0);
        return (token_t){ .type = TOK_IMM, .text = sv, .value = imm_val, .line = start_line, .col = start_col };
    }

    while (l->cursor < l->len && !isspace((unsigned char)l->src[l->cursor]) && l->src[l->cursor] != ',' && l->src[l->cursor] != ';' && l->src[l->cursor] != '#') {
        l->cursor++;
        l->col++;
    }

    StringView sv = { .data = start_ptr, .len = l->cursor - start_cursor };

    if (sv_equals(sv, "A") || sv_equals(sv, "rA") || sv_equals(sv, "R0")) return (token_t){ .type = TOK_REG, .text = sv, .value = REG_A, .line = start_line, .col = start_col };
    if (sv_equals(sv, "B") || sv_equals(sv, "rB") || sv_equals(sv, "R1")) return (token_t){ .type = TOK_REG, .text = sv, .value = REG_B, .line = start_line, .col = start_col };
    if (sv_equals(sv, "C") || sv_equals(sv, "rC") || sv_equals(sv, "R2")) return (token_t){ .type = TOK_REG, .text = sv, .value = REG_C, .line = start_line, .col = start_col };
    if (sv_equals(sv, "D") || sv_equals(sv, "rD") || sv_equals(sv, "R3")) return (token_t){ .type = TOK_REG, .text = sv, .value = REG_D, .line = start_line, .col = start_col };
    if (sv_equals(sv, "E") || sv_equals(sv, "rE") || sv_equals(sv, "R4")) return (token_t){ .type = TOK_REG, .text = sv, .value = REG_E, .line = start_line, .col = start_col };

    if (sv_equals(sv, "LABEL")) return (token_t){ .type = TOK_LABEL_KW, .text = sv, .value = NOOP_LABEL, .line = start_line, .col = start_col };

    if (sv_equals(sv, "NOP"))   return (token_t){ .type = TOK_OPCODE, .text = sv, .value = OP_NOP,   .line = start_line, .col = start_col };
    if (sv_equals(sv, "ADD"))   return (token_t){ .type = TOK_OPCODE, .text = sv, .value = OP_ADD,   .line = start_line, .col = start_col };
    if (sv_equals(sv, "SUB"))   return (token_t){ .type = TOK_OPCODE, .text = sv, .value = OP_SUB,   .line = start_line, .col = start_col };
    if (sv_equals(sv, "LOAD"))  return (token_t){ .type = TOK_OPCODE, .text = sv, .value = OP_LOAD,  .line = start_line, .col = start_col };
    if (sv_equals(sv, "STORE")) return (token_t){ .type = TOK_OPCODE, .text = sv, .value = OP_STORE, .line = start_line, .col = start_col };
    if (sv_equals(sv, "XOR"))   return (token_t){ .type = TOK_OPCODE, .text = sv, .value = OP_XOR,   .line = start_line, .col = start_col };
    if (sv_equals(sv, "AND"))   return (token_t){ .type = TOK_OPCODE, .text = sv, .value = OP_AND,   .line = start_line, .col = start_col };
    if (sv_equals(sv, "OR"))    return (token_t){ .type = TOK_OPCODE, .text = sv, .value = OP_OR,    .line = start_line, .col = start_col };
    if (sv_equals(sv, "NOT"))   return (token_t){ .type = TOK_OPCODE, .text = sv, .value = OP_NOT,   .line = start_line, .col = start_col };
    if (sv_equals(sv, "CMP"))   return (token_t){ .type = TOK_OPCODE, .text = sv, .value = OP_CMP,   .line = start_line, .col = start_col };
    if (sv_equals(sv, "JMP"))   return (token_t){ .type = TOK_OPCODE, .text = sv, .value = OP_JMP,   .line = start_line, .col = start_col };
    if (sv_equals(sv, "JEQ"))   return (token_t){ .type = TOK_OPCODE, .text = sv, .value = OP_JEQ,   .line = start_line, .col = start_col };
    if (sv_equals(sv, "MOV"))   return (token_t){ .type = TOK_OPCODE, .text = sv, .value = OP_MOV,   .line = start_line, .col = start_col };
    if (sv_equals(sv, "PRINT")) return (token_t){ .type = TOK_OPCODE, .text = sv, .value = OP_PRINT, .line = start_line, .col = start_col };

    return (token_t){ .type = TOK_IDENTIFIER, .text = sv, .value = 0, .line = start_line, .col = start_col };
}

static instruction_t *parse_sni(const char *source, size_t len, size_t *out_prg_size) {
    lexer_t lexer = lexer_init(source, len);

    size_t capacity = 32;
    size_t count = 0;
    instruction_t *prg = malloc(sizeof(instruction_t) * capacity);
    if (!prg) return NULL;

    token_t tok = lexer_next(&lexer);
    while (tok.type != TOK_EOF) {
        if (tok.type == TOK_LABEL_KW) {
            token_t id = lexer_next(&lexer);
            if (id.type != TOK_IMM) {
                fprintf(stderr, "[%u:%u] Syntax Error: Expected label numeric ID\n", id.line, id.col);
                free(prg);
                return NULL;
            }

            instruction_t inst = {
                .operation = NOOP_LABEL,
                .argument  = id.value
            };

            if (count >= capacity) {
                capacity *= 2;
                instruction_t *new_prg = realloc(prg, sizeof(instruction_t) * capacity);
                if (!new_prg) {
                    free(prg);
                    return NULL;
                }
                prg = new_prg;
            }
            prg[count++] = inst;
            tok = lexer_next(&lexer);
            continue;
        }

        if (tok.type != TOK_OPCODE) {
            fprintf(stderr, "[%u:%u] Syntax Error: Expected opcode, got '%.*s'\n",
                    tok.line, tok.col, (int)tok.text.len, tok.text.data);
            free(prg);
            return NULL;
        }

        uint8_t op = tok.value;
        uint8_t reg_dest = REG_NONE;
        uint8_t arg_val = 0;
        uint8_t is_imm = 0;

        if (op == OP_NOT) {
            token_t r = lexer_next(&lexer);
            if (r.type != TOK_REG) {
                fprintf(stderr, "[%u:%u] Syntax Error: NOT expects a register\n", r.line, r.col);
                free(prg);
                return NULL;
            }
            reg_dest = r.value;
        } else if (op == OP_PRINT) {
            token_t arg = lexer_next(&lexer);
            if (arg.type == TOK_REG) {
                reg_dest = arg.value;
            } else if (arg.type == TOK_IMM) {
                is_imm = IMM_FLAG;
                arg_val = arg.value;
            } else {
                fprintf(stderr, "[%u:%u] Syntax Error: PRINT expects register or immediate\n", arg.line, arg.col);
                free(prg);
                return NULL;
            }
        } else if (op == OP_JMP || op == OP_JEQ) {
            token_t arg = lexer_next(&lexer);
            if (arg.type != TOK_IMM) {
                fprintf(stderr, "[%u:%u] Syntax Error: Jump expects label ID\n", arg.line, arg.col);
                free(prg);
                return NULL;
            }
            arg_val = arg.value;
        } else {
            token_t r1 = lexer_next(&lexer);
            if (r1.type != TOK_REG) {
                fprintf(stderr, "[%u:%u] Syntax Error: Expected register as first operand\n", r1.line, r1.col);
                free(prg);
                return NULL;
            }
            reg_dest = r1.value;

            token_t r2 = lexer_next(&lexer);
            if (r2.type == TOK_REG) {
                arg_val = r2.value;
            } else if (r2.type == TOK_IMM) {
                is_imm = IMM_FLAG;
                arg_val = r2.value;
            } else {
                fprintf(stderr, "[%u:%u] Syntax Error: Expected second argument (reg or imm)\n", r2.line, r2.col);
                free(prg);
                return NULL;
            }
        }

        instruction_t inst;
        inst.operation = is_imm | reg_dest | op;
        inst.argument = arg_val;

        if (count >= capacity) {
            capacity *= 2;
            instruction_t *new_prg = realloc(prg, sizeof(instruction_t) * capacity);
            if (!new_prg) {
                free(prg);
                return NULL;
            }
            prg = new_prg;
        }
        prg[count++] = inst;

        tok = lexer_next(&lexer);
    }

    *out_prg_size = count;
    return prg;
}

static inline void usage(void){
    puts("Usage: ni [-c | -e] <file>");
    puts("  -c  Compile to bytecode dump (hex bytes)");
    puts("  -e  Execute the program");
}

static void dump_bytecode(instruction_t *prg, size_t prg_size) {
    for (size_t i = 0; i < prg_size; i++) {
        printf("[%04zu] 0x%02X 0x%02X\n", i, prg[i].operation, prg[i].argument);
    }
}

static void execp(instruction_t *prg, size_t prg_size){
    uint8_t reg_file[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t mem[256] = {0};
    int8_t eq = 1;

    size_t label_table[256];
    memset(label_table, 0xFF, sizeof(label_table));

    for (size_t i = 0; i < prg_size; i++) {
        if (prg[i].operation == NOOP_LABEL) {
            label_table[prg[i].argument] = i;
        }
    }

    for (size_t i = 0; i < prg_size; i++){
        instruction_t inst = prg[i];

        if (inst.operation == NOOP_LABEL) {
            continue;
        }

        uint8_t op = inst.operation & 0x0F;
        uint8_t imm = (inst.operation & IMM_FLAG) ? 1 : 0;

        uint8_t reg1 = 0, reg2 = 0;
        uint8_t reg_field = inst.operation & 0xF0;
        if (reg_field == REG_A)      reg1 = 0;
        else if (reg_field == REG_B) reg1 = 1;
        else if (reg_field == REG_C) reg1 = 2;
        else if (reg_field == REG_D) reg1 = 3;
        else if (reg_field == REG_E) reg1 = 4;

        if (!imm){
            if (inst.argument == REG_A)      reg2 = 0;
            else if (inst.argument == REG_B) reg2 = 1;
            else if (inst.argument == REG_C) reg2 = 2;
            else if (inst.argument == REG_D) reg2 = 3;
            else if (inst.argument == REG_E) reg2 = 4;
        }

        if (op == OP_ADD){
            uint8_t val = imm ? inst.argument : reg_file[reg2];
            uint16_t res = (uint16_t)reg_file[reg1] + (uint16_t)val;
            reg_file[reg1] = (res > 255) ? (uint8_t)(res - 256) : (uint8_t)res;
        }
        else if (op == OP_SUB){
            uint8_t val = imm ? inst.argument : reg_file[reg2];
            int16_t res = (int16_t)reg_file[reg1] - (int16_t)val;
            reg_file[reg1] = (res < 0) ? (uint8_t)(256 + res) : (uint8_t)res;
        }
        else if (op == OP_LOAD){
            uint8_t idx = imm ? inst.argument : reg_file[reg2];
            reg_file[reg1] = mem[idx];
        }
        else if (op == OP_STORE){
            uint8_t idx = imm ? inst.argument : reg_file[reg2];
            mem[idx] = reg_file[reg1];
        }
        else if (op == OP_XOR){
            uint8_t val = imm ? inst.argument : reg_file[reg2];
            reg_file[reg1] ^= val;
        }
        else if (op == OP_AND){
            uint8_t val = imm ? inst.argument : reg_file[reg2];
            reg_file[reg1] &= val;
        }
        else if (op == OP_OR){
            uint8_t val = imm ? inst.argument : reg_file[reg2];
            reg_file[reg1] |= val;
        }
        else if (op == OP_NOT){
            reg_file[reg1] = ~reg_file[reg1];
        }
        else if (op == OP_CMP){
            uint8_t val = imm ? inst.argument : reg_file[reg2];
            eq = reg_file[reg1] - val;
        }
        else if (op == OP_JMP){
            uint8_t target = inst.argument;
            if (label_table[target] == (size_t)-1) {
                fprintf(stderr, "Runtime Error: Undefined label %u\n", target);
                return;
            }
            i = label_table[target];
            continue;
        }
        else if (op == OP_JEQ){
            if (eq == 0) {
                uint8_t target = inst.argument;
                if (label_table[target] == (size_t)-1) {
                    fprintf(stderr, "Runtime Error: Undefined label %u\n", target);
                    return;
                }
                i = label_table[target];
            }
            continue;
        }
        else if (op == OP_MOV){
            uint8_t val = imm ? inst.argument : reg_file[reg2];
            reg_file[reg1] = val;
        }
        else if (op == OP_PRINT){
            uint8_t val = imm ? inst.argument : reg_file[reg1];
            putc((char)val, stdout);
            fflush(stdout);
        }
    }
}

filetype_t extension_checker(const char* filename){
    const char *dot = strrchr(filename, '.');

    if (!dot || dot == filename){
        return INVALID_TYPE;
    }

    if (strcmp(dot, ".ni") == 0){
        return NI_TYPE;
    } else if (strcmp(dot, ".sni") == 0){
        return SNI_TYPE;
    } else {
        return INVALID_TYPE;
    }
}

typedef enum {
    MODE_NONE,
    MODE_COMPILE,
    MODE_EXEC
} run_mode_t;

int main(int argc, char **argv){
    if (argc < 2){
        usage();
        return 0;
    }

    run_mode_t mode = MODE_NONE;
    const char *filepath = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            mode = MODE_COMPILE;
        } else if (strcmp(argv[i], "-e") == 0) {
            mode = MODE_EXEC;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown flag: %s\n", argv[i]);
            usage();
            return 1;
        } else {
            if (filepath != NULL) {
                fputs("Too many file arguments\n", stderr);
                usage();
                return 1;
            }
            filepath = argv[i];
        }
    }

    if (!filepath || mode == MODE_NONE) {
        usage();
        return 1;
    }

    filetype_t ft = extension_checker(filepath);
    if (ft == INVALID_TYPE) {
        fputs("Error: file extension unsupported\n", stderr);
        return 1;
    }

    FILE *f = fopen(filepath, ft == NI_TYPE ? "rb" : "r");
    if (f == NULL){
        fprintf(stderr, "Could not open file: %s\n", filepath);
        return 1;
    }
    if (fseek(f, 0, SEEK_END) != 0){
        fclose(f);
        fputs("File: Empty\n", stderr);
        return 1;
    }
    long length = ftell(f);
    if (length < 0) {
        fclose(f);
        return 1;
    }
    rewind(f);

    instruction_t *prg = NULL;
    size_t prg_size = 0;

    if (ft == NI_TYPE){
        prg_size = (size_t)length / sizeof(instruction_t);
        prg = malloc(sizeof(instruction_t) * prg_size);
        if (!prg){
            fclose(f);
            fputs("Memory error!\n", stderr);
            return 1;
        }
        fread(prg, sizeof(instruction_t), prg_size, f);
        fclose(f);
    } else if (ft == SNI_TYPE){
        char *buffer = malloc((size_t)length + 1);
        if (!buffer){
            fclose(f);
            fputs("Memory error!\n", stderr);
            return 1;
        }
        size_t bytes_read = fread(buffer, 1, (size_t)length, f);
        buffer[bytes_read] = '\0';
        fclose(f);

        prg = parse_sni(buffer, bytes_read, &prg_size);
        free(buffer);

        if (!prg) {
            return 1;
        }
    }

    if (mode == MODE_COMPILE) {
        dump_bytecode(prg, prg_size);
    } else if (mode == MODE_EXEC) {
        execp(prg, prg_size);
    }

    free(prg);
    return 0;
}
