#include "assembler.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#define ERROR_PTR ((char *)-1)

// Definition of an instruction format
typedef struct {
    const char* name;    // Instruction name
    int opcode;          // Opcode bits
    int func;            // Function bits (if any)
    int arg_count;       // Number of expected arguments
    char arg_types[3];   // Argument types: 'r' = register, 'i' = immediate
    int imm_bits;        // Immediate bit size (12 or 20 for our ISA)
} instruction_def_t;

// Supported instructions
static const instruction_def_t instructions[] = {
    {"ld",  0b0100011, 0, 2, {'r', 'r'}, 0},
    {"sd",  0b0100011, 1, 2, {'r', 'r'}, 0},
    {"li",  0b0100010, 0, 2, {'r', 'i'}, 20},
    {"not", 0b0110011, 0, 2, {'r', 'r'}, 0},
    {"and", 0b0110011, 1, 3, {'r', 'r', 'r'}, 0},
    {"or",  0b0110011, 2, 3, {'r', 'r', 'r'}, 0},
    {"add", 0b0110011, 4, 3, {'r', 'r', 'r'}, 0},
    {"sub", 0b0110011, 5, 3, {'r', 'r', 'r'}, 0},
    {"inc", 0b0110011, 6, 2, {'r', 'r'}, 0},
    {"j",   0b1100011, 0, 1, {'r'}, 0},
    {"beq", 0b1100011, 1, 3, {'r', 'r', 'i'}, 12},
    {"bne", 0b1100011, 2, 3, {'r', 'r', 'i'}, 12},
    {"blt", 0b1100011, 3, 3, {'r', 'r', 'i'}, 12},
    {"bgt", 0b1100011, 4, 3, {'r', 'r', 'i'}, 12},
    {NULL, 0, 0, 0, {'\0'}, 0} // End marker
};

// Parse a number with base detection (hex, binary, decimal)
static long parse_number_with_base(const char* arg, int* base_out) {
    if (arg[0] == '0' && arg[1] == 'x') {
        *base_out = 16;
        return strtol(arg + 2, NULL, 16);
    } else if (arg[0] == '0' && arg[1] == 'b') {
        *base_out = 2;
        return strtol(arg + 2, NULL, 2);
    } else {
        *base_out = 10;
        return strtol(arg, NULL, 10);
    }
}

// Validate if the string is a legal number for given bit-width
short checkIfValidNumber(char *arg, short is20bit) {
    char *rest = NULL;
    int base;
    
    // No unnecessary leading zeros (except in base-prefixed numbers)
    if (arg[0] == '0' && arg[1] != 'x' && arg[1] != 'b' && arg[1] != '\0')
        return 0;

    if ((arg[0] == '0' && arg[1] == 'x' && arg[2] == '0' && arg[3] != '\0') ||
        (arg[0] == '0' && arg[1] == 'b' && arg[2] == '0' && arg[3] != '\0'))
        return 0;

    // Parse number
    parse_number_with_base(arg, &base);
    if (base == 16)
        strtol(arg + 2, &rest, 16);
    else if (base == 2)
        strtol(arg + 2, &rest, 2);
    else
        strtol(arg, &rest, 10);

    // Invalid format
    if (arg == rest || *rest != '\0') return 0;

    // Check range
    long max_val = is20bit ? (1 << 20) - 1 : (1 << 12) - 1;
    long val = parse_number_with_base(arg, &base);
    return (val >= 0 && val <= max_val);
}

// Convert a number string to integer
int convertNumberToInt(char *arg) {
    int base;
    return (int)parse_number_with_base(arg, &base);
}

// Find instruction definition by name
const instruction_def_t* find_instruction(const char* name) {
    for (int i = 0; instructions[i].name; i++) {
        if (strcmp(name, instructions[i].name) == 0)
            return &instructions[i];
    }
    return NULL;
}

// Check if a string is a valid register (x0-x31)
short checkIfValidRegister(char *arg) {
    if (arg[0] != 'x') return 0;
    char* rest = NULL;
    int value = strtol(arg + 1, &rest, 10);

    // No unnecessary leading zeros
    if (arg[1] == '0' && arg[2] != '\0') return 0;

    return (*rest == '\0' && rest != arg + 1 && value >= 0 && value <= 31);
}

// Remove comments (#) and extra empty lines
char *remove_comments(const char *input) {
    char *clean = malloc(strlen(input) + 1);
    if (!clean) return NULL;
    char *dst = clean;

    // Remove comments
    for (const char *src = input; *src; src++) {
        if (*src == '#') {
            while (*src && *src != '\n') src++;
            if (!*src) break;
        }
        *dst++ = *src;
    }
    *dst = '\0';

    // Remove extra empty lines
    char *finalBuffer = malloc(strlen(clean) + 1);
    if (!finalBuffer) { free(clean); return NULL; }
    char *out = finalBuffer;
    char *ptr = clean;

    while (*ptr == '\n') ptr++; // Skip leading newlines
    while (*ptr) {
        if (*ptr == '\n') {
            *out++ = *ptr++;
            while (*ptr == '\n') ptr++;
        } else {
            *out++ = *ptr++;
        }
    }
    if (out > finalBuffer && *(out - 1) == '\n') out--;
    *out = '\0';

    free(clean);
    return finalBuffer;
}

// Skip spaces/tabs
static const char* skip_whitespace(const char* ptr) {
    while (*ptr == ' ' || *ptr == '\t') ptr++;
    return ptr;
}

// Parse token until space, tab, newline, or delimiter
static const char* parse_token(const char* src, char* dest, char delimiter) {
    src = skip_whitespace(src);
    while (*src && *src != delimiter && *src != ' ' && *src != '\t' && *src != '\n') {
        *dest++ = *src++;
    }
    *dest = '\0';
    return src;
}

// Extract one line into instruction + up to 3 args
char *split_next_line(const char *code, char *instruction, char *arg1, char *arg2, char *arg3) {
    *instruction = *arg1 = *arg2 = *arg3 = '\0';
    code = skip_whitespace(code);

    code = parse_token(code, instruction, ' ');
    if (!*instruction) return ERROR_PTR;

    code = skip_whitespace(code);
    code = parse_token(code, arg1, ',');
    code = skip_whitespace(code);

    if (*code == ',') {
        code = parse_token(skip_whitespace(++code), arg2, ',');
        code = skip_whitespace(code);
        if (*code == ',') {
            code = parse_token(skip_whitespace(++code), arg3, ',');
        }
    }

    const instruction_def_t* def = find_instruction(instruction);
    if (!def) { printf("Unknown instruction: %s\n", instruction); return ERROR_PTR; }

    char* args[] = {arg1, arg2, arg3};
    for (int i = 0; i < def->arg_count; i++) {
        if (def->arg_types[i] == 'r' && !checkIfValidRegister(args[i])) {
            printf("Invalid register: %s\n", args[i]); return ERROR_PTR;
        }
        if (def->arg_types[i] == 'i' && !checkIfValidNumber(args[i], def->imm_bits == 20)) {
            printf("Invalid immediate: %s\n", args[i]); return ERROR_PTR;
        }
    }
    for (int i = def->arg_count; i < 3; i++) {
        if (*args[i]) { printf("Too many arguments for %s\n", instruction); return ERROR_PTR; }
    }

    while (*code && *code != '\n') code++;
    return *code == '\n' ? (char*)(code + 1) : NULL;
}

// Encode a single instruction into binary format
instruction_t assemble_single_instruction(char *instruction, char *arg1, char *arg2, char *arg3) {
    const instruction_def_t* def = find_instruction(instruction);
    if (!def) return 0;

    instruction_t result = def->opcode;
    if (def->func) result |= (def->func << 12);

    if (def->imm_bits == 12 && def->arg_count == 3) { // B-type
        int rs1 = atoi(arg1 + 1);
        int rs2 = atoi(arg2 + 1);
        int imm = convertNumberToInt(arg3);
        result |= (rs1 << 15) | (rs2 << 20) | ((imm >> 5) << 25) | ((imm & 0x1F) << 7);
    } else if (def->imm_bits == 20) { // U-type
        int rd = atoi(arg1 + 1);
        int imm = convertNumberToInt(arg2);
        result |= (rd << 7) | (imm << 12);
    } else { // R-type and special cases
        if (strcmp(instruction, "sd") == 0) {
            int rs1 = atoi(arg1 + 1);
            int rs2 = atoi(arg2 + 1);
            result |= (rs1 << 20) | (rs2 << 15);
        } else {
            if (*arg1) result |= (atoi(arg1 + 1) << 7);
            if (*arg2) result |= (atoi(arg2 + 1) << 15);
            if (*arg3) result |= (atoi(arg3 + 1) << 20);
        }
    }
    return result;
}

// Assemble all instructions from input string
size_t assemble(const char *input, instruction_t *output) {
    char *code = remove_comments(input);
    if (!code) return 0;

    char *instruction = malloc(20), *arg1 = malloc(20), *arg2 = malloc(20), *arg3 = malloc(20);
    if (!instruction || !arg1 || !arg2 || !arg3) { free(instruction); free(arg1); free(arg2); free(arg3); free(code); return 0; }

    int count = 0;
    while (code) {
        code = split_next_line(code, instruction, arg1, arg2, arg3);
        if (code == ERROR_PTR) { free(instruction); free(arg1); free(arg2); free(arg3); free(code); return 0; }
        if (output) *output++ = assemble_single_instruction(instruction, arg1, arg2, arg3);
        count++;
    }

    free(instruction); free(arg1); free(arg2); free(arg3); free(code);
    return count;
}