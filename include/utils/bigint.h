#pragma once
#include <tommath.h>
#include <string>
#include <stdexcept>
#include <climits>
#include <iostream>
#include <cstdint>  // Add this for uint64_t
#include <limits>   // Add this for std::numeric_limits

class BigInt {
    mp_int value;
    
public:
    BigInt() {
        mp_init(&value);
    }
    
    BigInt(int64_t val) {
        mp_init(&value);
        mp_set_i64(&value, val);
    }
    
    BigInt(const std::string& str) {
        mp_init(&value);
        if (mp_read_radix(&value, str.c_str(), 10) != MP_OKAY) {
            throw std::runtime_error("Invalid integer: " + str);
        }
    }
    
    BigInt(const BigInt& other) {
        mp_init_copy(&value, const_cast<mp_int*>(&other.value));
    }
    
    BigInt& operator=(const BigInt& other) {
        if (this != &other) {
            mp_clear(&value);
            mp_init_copy(&value, const_cast<mp_int*>(&other.value));
        }
        return *this;
    }
    
    ~BigInt() {
        mp_clear(&value);
    }
    
    bool fitsInInt64() const {
        int bits = mp_count_bits(&value);

        if (bits > 63) {
            if (mp_isneg(&value) && bits == 64) {
                BigInt int64_min("-9223372036854775808");
                return *this == int64_min;
            }
            return false;
        }
        
        return true;
    }
    
    int64_t toInt64() const {
        if (!fitsInInt64()) {
            std::cerr << "DEBUG: Value doesn't fit in int64, throwing error" << std::endl;
            throw std::runtime_error("Integer out of range for int64: " + toString() + 
                                    ". Valid range: -9223372036854775808 to 9223372036854775807");
        }
        
        uint64_t magnitude = 0;
        
        #if defined(MP_GET_U64)
            mp_get_u64(&value, &magnitude);
        #else
            for (int i = 0; i < value.used; i++) {
                magnitude |= ((uint64_t)value.dp[i]) << (i * MP_DIGIT_BIT);
            }
        #endif
        
        if (mp_isneg(&value)) {
            if (magnitude == 9223372036854775808ULL) {
                return INT64_MIN;
            }
            return -static_cast<int64_t>(magnitude);
        }
        
        return static_cast<int64_t>(magnitude);
    }
    
    bool fitsInType(int type) const {
        switch (type) {
            case 0:
                return *this >= MIN_INT8 && *this <= MAX_INT8;
            case 1:
                return *this >= MIN_INT16 && *this <= MAX_INT16;
            case 2:
                return *this >= MIN_INT32 && *this <= MAX_INT32;
            case 3:
                return fitsInInt64();
            default:
                return false;
        }
    }
    
    std::string toString() const {
        int size;
        mp_radix_size(&value, 10, &size);
        std::string result(size, '\0');
        mp_toradix(&value, &result[0], 10);
        
        if (!result.empty() && result.back() == '\0') {
            result.pop_back();
        }
        return result;
    }
    
    bool operator<(const BigInt& other) const {
        return mp_cmp(&value, const_cast<mp_int*>(&other.value)) == MP_LT;
    }
    
    bool operator<=(const BigInt& other) const {
        int cmp = mp_cmp(&value, const_cast<mp_int*>(&other.value));
        return cmp == MP_LT || cmp == MP_EQ;
    }
    
    bool operator>(const BigInt& other) const {
        return mp_cmp(&value, const_cast<mp_int*>(&other.value)) == MP_GT;
    }
    
    bool operator>=(const BigInt& other) const {
        int cmp = mp_cmp(&value, const_cast<mp_int*>(&other.value));
        return cmp == MP_GT || cmp == MP_EQ;
    }
    
    bool operator==(const BigInt& other) const {
        return mp_cmp(&value, const_cast<mp_int*>(&other.value)) == MP_EQ;
    }
    
    bool operator!=(const BigInt& other) const {
        return mp_cmp(&value, const_cast<mp_int*>(&other.value)) != MP_EQ;
    }

    // REMOVED the problematic toUint64() method
    
    static const BigInt MIN_INT4;
    static const BigInt MAX_INT4;
    static const BigInt MIN_INT8;
    static const BigInt MAX_INT8;
    static const BigInt MIN_INT12;
    static const BigInt MAX_INT12;
    static const BigInt MIN_INT16;
    static const BigInt MAX_INT16;
    static const BigInt MIN_INT24;
    static const BigInt MAX_INT24;
    static const BigInt MIN_INT32;
    static const BigInt MAX_INT32;
    static const BigInt MIN_INT48;
    static const BigInt MAX_INT48;
    static const BigInt MIN_INT64;
    static const BigInt MAX_INT64;
    static const BigInt MAX_UINT8;
    static const BigInt MAX_UINT16;
    static const BigInt MAX_UINT32;
    static const BigInt MAX_UINT64;
    static const BigInt MIN_UINT0;
    static const BigInt MAX_UINT0;
    static const BigInt MIN_UINT4;
    static const BigInt MAX_UINT4;
    static const BigInt MIN_UINT12;
    static const BigInt MAX_UINT12;
    static const BigInt MIN_UINT24;
    static const BigInt MAX_UINT24;
    static const BigInt MIN_UINT48;
    static const BigInt MAX_UINT48;
    
private:
    static BigInt createFromInt64(int64_t val) {
        BigInt result;
        mp_set_i64(&result.value, val);
        return result;
    }
};