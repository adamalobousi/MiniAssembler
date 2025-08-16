#include <stdio.h>
#include <string.h>
#include "assembler.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <getopt.h>

// Read the full content of a file into a string buffer
char *read_file(const char *path) {
    char *buffer = NULL;
    FILE *file = fopen(path, "r");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    struct stat statbuf;
    if (fstat(fileno(file), &statbuf)) {
        fprintf(stderr, "Error retrieving file stats\n");
        goto cleanup;
    }

    // Check if it's a valid regular file with non-zero size
    if (!S_ISREG(statbuf.st_mode) || statbuf.st_size <= 0) {
        fprintf(stderr, "Invalid file: Not a regular file or size is 0\n");
        goto cleanup;
    }

    // Allocate buffer to hold file contents (+1 for null terminator)
    buffer = malloc(statbuf.st_size + 1);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed for file content\n");
        goto cleanup;
    }

    // Read file into buffer
    if (fread(buffer, 1, statbuf.st_size, file) != (size_t)statbuf.st_size) {
        fprintf(stderr, "Error reading file\n");
        free(buffer);
        buffer = NULL;
        goto cleanup;
    }

    buffer[statbuf.st_size] = '\0'; // Null-terminate the string

cleanup:
    fclose(file);
    return buffer;
}

// Write a string to a file
int write_file(const char *path, const char *string) {
    FILE *file = fopen(path, "w");
    if (!file) {
        perror("Error opening file for writing");
        return 1;
    }

    size_t len = strlen(string);
    if (fwrite(string, 1, len, file) != len) {
        fprintf(stderr, "Error writing to file\n");
        fclose(file);
        return 1;
    }

    fclose(file);
    return 0;
}

int main(int argc, char** argv) {
    static struct option long_options[] = {
        {"help",   no_argument,       0, 'h'},
        {"output", required_argument, 0, 'o'},
        {0, 0, 0, 0}
    };

    int option_val;
    char *outputPath = NULL;
    char *inputPath = NULL;

    // Parse command-line options
    while ((option_val = getopt_long(argc, argv, "ho:", long_options, NULL)) != -1) {
        switch (option_val) {
            case 'h':
                printf("Usage: assembler [options] <inputfile>\n"
                       "Options:\n"
                       "  -h, --help        Show this help message\n"
                       "  -o, --output FILE Set output file path (default: bin/output)\n");
                return 0;
            case 'o':
                outputPath = optarg;
                break;
            default:
                return 1;
        }
    }

    // Remaining argument should be the input file
    if (optind >= argc) {
        fprintf(stderr, "Error: Missing input file\n");
        return 1;
    }
    inputPath = argv[optind];

    // Default output path if not specified
    if (!outputPath)
        outputPath = "bin/output";

    // Read input file
    char *input = read_file(inputPath);
    if (!input) {
        fprintf(stderr, "Could not read input file\n");
        return 1;
    }

    // First pass: get number of instructions
    size_t output_size = assemble(input, NULL);
    if (output_size == 0) {
        fprintf(stderr, "Failed to assemble input\n");
        free(input);
        return 1;
    }

    // Allocate memory for binary output
    instruction_t *output = malloc(sizeof(instruction_t) * output_size);
    if (!output) {
        fprintf(stderr, "Memory allocation failed for output\n");
        free(input);
        return 1;
    }

    // Second pass: assemble into binary
    assemble(input, output);

    // Convert binary instructions to string of '0'/'1'
    char *binary_str = malloc(output_size * 33 + output_size); // 32 bits + newline
    if (!binary_str) {
        fprintf(stderr, "Memory allocation failed for binary string\n");
        free(output);
        free(input);
        return 1;
    }
    char *ptr = binary_str;

    for (size_t i = 0; i < output_size; i++) {
        for (int bit = 31; bit >= 0; bit--)
            *ptr++ = ((output[i] >> bit) & 1) ? '1' : '0';
        *ptr++ = '\n';
    }
    *(ptr - 1) = '\0'; // Remove last newline and terminate string

    // Write binary string to output file
    if (write_file(outputPath, binary_str) != 0) {
        fprintf(stderr, "Could not write to output file\n");
        free(binary_str);
        free(output);
        free(input);
        return 1;
    }

    // Cleanup
    free(binary_str);
    free(output);
    free(input);
    return 0;
}