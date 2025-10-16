#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

#if defined(_WIN32) || defined(_WIN64)
#ifndef GETLINE_DEFINED
#define GETLINE_DEFINED

ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
    char *bufptr = NULL;
    char *p = bufptr;
    size_t size;
    int c;

    if (lineptr == NULL || stream == NULL || n == NULL) {
        return -1;
    }

    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF) {
        return -1;
    }
    
    if (bufptr == NULL) {
        bufptr = malloc(128);
        if (bufptr == NULL) {
            return -1;
        }
        size = 128;
    }
    
    p = bufptr;
    while(c != EOF) {
        if ((p - bufptr) > (size - 1)) {
            size = size + 128;
            bufptr = realloc(bufptr, size);
            if (bufptr == NULL) {
                return -1;
            }
            p = bufptr + (p - bufptr);
        }
        *p++ = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }

    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;

    return p - bufptr - 1;
}

#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef long long ssize_t;
#endif

#endif
#endif

void io_print_str(const char* str) {
    if (str) {
        printf("%s", str);
        fflush(stdout);
    }
}

void io_println_str(const char* str) {
    if (str) {
        printf("%s\n", str);
    } else {
        printf("\n");
    }
    fflush(stdout);
}

char* io_readln() {
    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    
    read = getline(&line, &len, stdin);
    if (read == -1) {
        free(line);
        return NULL;
    }
    
    if (read > 0 && line[read - 1] == '\n') {
        line[read - 1] = '\0';
    }
    
    return line;
}

uint64_t io_read_int() {
    char* input = io_readln();
    if (!input) {
        fprintf(stderr, "Error reading integer input\n");
        return 0;
    }
    
    char* ptr = input;
    while (isspace(*ptr)) ptr++;
    
    if (*ptr == '\0') {
        free(input);
        fprintf(stderr, "Empty input for integer\n");
        return 0;
    }
    
    bool is_negative = false;
    if (*ptr == '-') {
        is_negative = true;
        ptr++;
    }
    
    char* endptr;
    errno = 0;
    
    uint64_t value = strtoull(ptr, &endptr, 10);
    
    if (errno == ERANGE) {
        free(input);
        fprintf(stderr, "Integer value out of range\n");
        return 0;
    }
    
    while (isspace(*endptr)) endptr++;
    if (*endptr != '\0') {
        free(input);
        fprintf(stderr, "Invalid input: not a valid integer\n");
        return 0;
    }
    
    free(input);
    
    if (is_negative) {
        if (value > (uint64_t)INT64_MAX + 1) {
            fprintf(stderr, "Integer value out of range for negative numbers\n");
            return 0;
        }
        
        int64_t signed_value = -(int64_t)value;
        return (uint64_t)signed_value;
    }
    
    return value;
}

bool io_check_int4_bounds(int64_t value) {
    return value >= -8 && value <= 7;
}

bool io_check_int8_bounds(int64_t value) {
    return value >= INT8_MIN && value <= INT8_MAX;
}

bool io_check_int12_bounds(int64_t value) {
    return value >= -2048 && value <= 2047;
}

bool io_check_int16_bounds(int64_t value) {
    return value >= INT16_MIN && value <= INT16_MAX;
}

bool io_check_int24_bounds(int64_t value) {
    return value >= -8388608 && value <= 8388607;
}

bool io_check_int32_bounds(int64_t value) {
    return value >= INT32_MIN && value <= INT32_MAX;
}

bool io_check_int48_bounds(int64_t value) {
    return value >= -140737488355328LL && value <= 140737488355327LL;
}

bool io_check_int64_bounds(int64_t value) {
    return value >= INT64_MIN && value <= INT64_MAX;
}

bool io_check_uint0_bounds(int64_t value) {
    return value == 0;
}

bool io_check_uint4_bounds(int64_t value) {
    return value >= 0 && value <= 15;
}

bool io_check_uint8_bounds(int64_t value) {
    return value >= 0 && value <= UINT8_MAX;
}

bool io_check_uint12_bounds(int64_t value) {
    return value >= 0 && value <= 4095;
}

bool io_check_uint16_bounds(int64_t value) {
    return value >= 0 && value <= UINT16_MAX;
}

bool io_check_uint24_bounds(int64_t value) {
    return value >= 0 && value <= 16777215;
}

bool io_check_uint32_bounds(int64_t value) {
    return value >= 0 && value <= UINT32_MAX;
}

bool io_check_uint48_bounds(int64_t value) {
    return value >= 0 && value <= 281474976710655LL;
}

bool io_check_uint64_bounds(int64_t value) {
    return value >= 0 && value <= UINT64_MAX;
}

char* int4_to_string(int8_t value) {
    char* buffer = (char*)malloc(8);
    snprintf(buffer, 8, "%d", value);
    return buffer;
}

char* int8_to_string(int8_t value) {
    char* buffer = (char*)malloc(8);
    snprintf(buffer, 8, "%d", value);
    return buffer;
}

char* int12_to_string(int16_t value) {
    char* buffer = (char*)malloc(16);
    snprintf(buffer, 16, "%d", value);
    return buffer;
}

char* int16_to_string(int16_t value) {
    char* buffer = (char*)malloc(16);
    snprintf(buffer, 16, "%d", value);
    return buffer;
}

char* int24_to_string(int32_t value) {
    char* buffer = (char*)malloc(16);
    snprintf(buffer, 16, "%d", value);
    return buffer;
}

char* int32_to_string(int32_t value) {
    char* buffer = (char*)malloc(16);
    snprintf(buffer, 16, "%d", value);
    return buffer;
}

char* int48_to_string(int64_t value) {
    char* buffer = (char*)malloc(32);
    snprintf(buffer, 32, "%lld", (long long)value);
    return buffer;
}

char* int64_to_string(int64_t value) {
    printf("DEBUG: int64_to_string called with value: %lld\n", (long long)value);
    char* buffer = (char*)malloc(32);
    snprintf(buffer, 32, "%lld", (long long)value);
    printf("DEBUG: int64_to_string returning: %s\n", buffer);
    return buffer;
}

char* uint0_to_string(uint8_t value) {
    char* buffer = (char*)malloc(8);
    snprintf(buffer, 8, "%u", value);
    return buffer;
}

char* uint4_to_string(uint8_t value) {
    char* buffer = (char*)malloc(8);
    snprintf(buffer, 8, "%u", value);
    return buffer;
}

char* uint8_to_string(uint8_t value) {
    char* buffer = (char*)malloc(8);
    snprintf(buffer, 8, "%u", value);
    return buffer;
}

char* uint12_to_string(uint16_t value) {
    char* buffer = (char*)malloc(16);
    snprintf(buffer, 16, "%u", value);
    return buffer;
}

char* uint16_to_string(uint16_t value) {
    char* buffer = (char*)malloc(16);
    snprintf(buffer, 16, "%u", value);
    return buffer;
}

char* uint24_to_string(uint32_t value) {
    char* buffer = (char*)malloc(16);
    snprintf(buffer, 16, "%u", value);
    return buffer;
}

char* uint32_to_string(uint32_t value) {
    char* buffer = (char*)malloc(16);
    snprintf(buffer, 16, "%u", value);
    return buffer;
}

char* uint48_to_string(uint64_t value) {
    char* buffer = (char*)malloc(32);
    snprintf(buffer, 32, "%llu", (unsigned long long)value);
    return buffer;
}

char* uint64_to_string(uint64_t value) {
    printf("DEBUG: uint64_to_string called with value: %llu\n", (unsigned long long)value);
    char* buffer = (char*)malloc(32);
    snprintf(buffer, 32, "%llu", (unsigned long long)value);
    printf("DEBUG: uint64_to_string returning: %s\n", buffer);
    return buffer;
}

char* float_to_string(float value) {
    char* buffer = (char*)malloc(32);
    snprintf(buffer, 32, "%f", value);
    return buffer;
}

char* double_to_string(double value) {
    char* buffer = (char*)malloc(32);
    snprintf(buffer, 32, "%lf", value);
    return buffer;
}

char* bool_to_string(bool value) {
    char* buffer = (char*)malloc(6);
    strcpy(buffer, value ? "true" : "false");
    return buffer;
}