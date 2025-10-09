#include "bounds.h"
#include "ast/ast.h"

using namespace AST;

bool TypeBounds::checkBounds(VarType type, const BigInt& value) {
    switch (type) {
        case VarType::INT8:
            return value >= BigInt::MIN_INT8 && value <= BigInt::MAX_INT8;
        case VarType::INT16:
            return value >= BigInt::MIN_INT16 && value <= BigInt::MAX_INT16;
        case VarType::INT32:
            return value >= BigInt::MIN_INT32 && value <= BigInt::MAX_INT32;
        case VarType::INT64:
            return value >= BigInt::MIN_INT64 && value <= BigInt::MAX_INT64;
        default:
            return false;
    }
}

std::string TypeBounds::getTypeRange(VarType type) {
    switch (type) {
        case VarType::INT8:
            return BigInt::MIN_INT8.toString() + " to " + BigInt::MAX_INT8.toString();
        case VarType::INT16:
            return BigInt::MIN_INT16.toString() + " to " + BigInt::MAX_INT16.toString();
        case VarType::INT32:
            return BigInt::MIN_INT32.toString() + " to " + BigInt::MAX_INT32.toString();
        case VarType::INT64:
            return BigInt::MIN_INT64.toString() + " to " + BigInt::MAX_INT64.toString();
        default:
            return "unknown range";
    }
}

std::string TypeBounds::getTypeName(VarType type) {
    switch (type) {
        case VarType::INT8: return "int8";
        case VarType::INT16: return "int16";
        case VarType::INT32: return "int32";
        case VarType::INT64: return "int64";
        case VarType::VOID: return "void";
        default: return "unknown";
    }
}