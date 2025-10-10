#pragma once

namespace AST {
    enum class VarType {
        BOOL,
        INT4,
        INT8,
        INT12,
        INT16,
        INT24,
        INT32,
        INT48,
        INT64,
        UINT4,
        UINT8,
        UINT12,
        UINT16,
        UINT24,
        UINT32,
        UINT48,
        UINT64,
        UINT0,
        FLOAT32,
        FLOAT64,
        STRING,
        VOID
    };

    enum class BinaryOp {
        ADD,
        SUBTRACT,
        MULTIPLY,
        DIVIDE,
        MODULUS,
        GREATER,
        LESS,
        GREATER_EQUAL,
        LESS_EQUAL,
        EQUAL,
        NOT_EQUAL,
        LOGICAL_AND,
        LOGICAL_OR,
        BITWISE_AND,
        BITWISE_OR,
        BITWISE_XOR,
        LEFT_SHIFT,
        RIGHT_SHIFT
    };
    
    enum class UnaryOp {
        LOGICAL_NOT,
        NEGATE,
        BITWISE_NOT
    };
}