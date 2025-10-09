#pragma once

namespace AST {
    enum class VarType {
        INT8,
        INT16, 
        INT32,
        INT64,
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