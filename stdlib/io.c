#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

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

char* int8_to_string(int8_t value) {
    char* buffer = (char*)malloc(8);
    snprintf(buffer, 8, "%d", value);
    return buffer;
}

char* int16_to_string(int16_t value) {
    char* buffer = (char*)malloc(16);
    snprintf(buffer, 16, "%d", value);
    return buffer;
}

char* int32_to_string(int32_t value) {
    char* buffer = (char*)malloc(16);
    snprintf(buffer, 16, "%d", value);
    return buffer;
}

char* int64_to_string(int64_t value) {
    char* buffer = (char*)malloc(32);
    snprintf(buffer, 32, "%lld", (long long)value);
    return buffer;
}

char* uint8_to_string(uint8_t value) {
    char* buffer = (char*)malloc(8);
    snprintf(buffer, 8, "%u", value);
    return buffer;
}

char* uint16_to_string(uint16_t value) {
    char* buffer = (char*)malloc(16);
    snprintf(buffer, 16, "%u", value);
    return buffer;
}

char* uint32_to_string(uint32_t value) {
    char* buffer = (char*)malloc(16);
    snprintf(buffer, 16, "%u", value);
    return buffer;
}

char* uint64_to_string(uint64_t value) {
    char* buffer = (char*)malloc(32);
    snprintf(buffer, 32, "%llu", (unsigned long long)value);
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