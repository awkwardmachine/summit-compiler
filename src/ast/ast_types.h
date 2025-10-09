#pragma once

namespace AST {
    enum class VarType {
        INT4,
        INT8,
        INT12,
        INT16,
        INT24,
        INT32,
        INT48,
        INT64,
        UINT4,
        UINT12,
        UINT24,
        UINT48,
        UINT8,
        UINT16,
        UINT32,
        UINT64,
        UINT0,
        STRING,
        VOID
    };

    enum class BinaryOp {
        ADD,
        SUBTRACT,
        MULTIPLY,
        DIVIDE
    };
}