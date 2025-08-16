# TinyAssembler

TinyAssembler is a small command-line assembler that converts a simplified assembly language into binary machine code. This project is written in C and uses standard libraries for file I/O and command-line argument parsing.

---

## Features

- Reads assembly source files
- Converts instructions into 32-bit binary format
- Writes output to a specified file (default: `a.out`)
- Supports command-line options:
  - `-h, --help` : Display help message
  - `-o, --output FILE` : Specify output file path

---

## Supported Instructions

| Instruction| Format         | Opcode    | Funct | Description                   |
|------------|----------------|-----------|-------|-------------------------------|
| ld         | rd, rs1        | 0b0100011 | 0     | rd = memory[rs1]              |
| sd         | rs2, rs1       | 0b0100011 | 1     | memory[rs1] = rs2             |
| li         | rd, imm        | 0b0100010 |       | rd = imm                      |
| not        | rd, rs1        | 0b0110011 | 0     | rd = !rs1                     |
| and        | rd, rs1, rs2   | 0b0110011 | 1     | rd = rs1 && rs2               |
| or         | rd, rs1, rs2   | 0b0110011 | 2     | rd = rs1 || rs2               |
| add        | rd, rs1, rs2   | 0b0110011 | 4     | rd = rs1 + rs2                |
| sub        | rd, rs1, rs2   | 0b0110011 | 5     | rd = rs1 - rs2                |
| inc        | rd, rs1        | 0b0110011 | 6     | rd = rs1 + 1                  |
| j          | rd             | 0b1100011 | 0     | jump to rd                    |
| beq        | rs1, rs2, imm  | 0b1100011 | 1     | PC += imm if rs1 == rs2       |
| bne        | rs1, rs2, imm  | 0b1100011 | 2     | PC += imm if rs1 != rs2       |
| blt        | rs1, rs2, imm  | 0b1100011 | 3     | PC += imm if rs1 < rs2        |
| bgt        | rs1, rs2, imm  | 0b1100011 | 4     | PC += imm if rs1 > rs2        |

---

## Bytecode-Format

| 31 - 25 | 24 - 20 | 19 - 15 | 14 - 12 | 11 - 7 |  6 - 0 |  Typ   |
|---------|---------|---------|---------|--------|--------|--------|
| 0000000 |   rs2   |   rs1   |  funct  |   rd   | opcode | R-type |
|   imm   |   imm   |   imm   |   imm   |   rd   | opcode | U-type |
|imm[11:5]|   rs2   |   rs1   |  funct  |imm[4:0]| opcode | B-type |

---

## Example

**Input:**<br>
add x14, x15, x16<br>
ld x1, x2<br>
beq x23, x24, 8<br>

**Output:**<br>
00000001000001111100011100110011<br>
00000000000000010000000010100011<br>
00000001100010111001010001100011<br>

---

## Requirements

- GCC or Clang compiler
- Standard C libraries (`stdio.h`, `stdlib.h`, `string.h`, etc.)

---

## Build

Use `make` to compile the project:

```bash
# Build files
make

# Clean compiled files
make clean