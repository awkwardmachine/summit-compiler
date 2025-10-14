#include <math.h>
#include <stdint.h>

int32_t math_abs(int32_t x) {
    return (x < 0) ? -x : x;
}

float math_pow(float base, float exponent) {
    return powf(base, exponent);
}

float math_sqrt(float x) {
    return sqrtf(x);
}

float math_round(float x) {
    return roundf(x);
}

float math_min(float a, float b) {
    return (a < b) ? a : b;
}

float math_max(float a, float b) {
    return (a > b) ? a : b;
}