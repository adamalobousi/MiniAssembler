#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdint.h>
#include <stddef.h>

typedef uint32_t instruction_t;

size_t assemble(const char* input, instruction_t* output);

#endif