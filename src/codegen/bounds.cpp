#include "bounds.h"
#include "ast/ast.h"

using namespace AST;
bool TypeBounds::checkBounds(VarType type, const BigInt& value) {
    switch (type) {
        case VarType::BOOL:
            return value == BigInt("0") || value == BigInt("1");
        case VarType::INT4:
            return value >= BigInt::MIN_INT4 && value <= BigInt::MAX_INT4;
        case VarType::INT8:
            return value >= BigInt::MIN_INT8 && value <= BigInt::MAX_INT8;
        case VarType::INT12:
            return value >= BigInt::MIN_INT12 && value <= BigInt::MAX_INT12;
        case VarType::INT16:
            return value >= BigInt::MIN_INT16 && value <= BigInt::MAX_INT16;
        case VarType::INT24:
            return value >= BigInt::MIN_INT24 && value <= BigInt::MAX_INT24;
        case VarType::INT32:
            return value >= BigInt::MIN_INT32 && value <= BigInt::MAX_INT32;
        case VarType::INT48:
            return value >= BigInt::MIN_INT48 && value <= BigInt::MAX_INT48;
        case VarType::INT64:
            return value >= BigInt::MIN_INT64 && value <= BigInt::MAX_INT64;
        case VarType::UINT4:
            return value >= BigInt("0") && value <= BigInt::MAX_UINT4;
        case VarType::UINT8:
            return value >= BigInt("0") && value <= BigInt::MAX_UINT8;
        case VarType::UINT12:
            return value >= BigInt("0") && value <= BigInt::MAX_UINT12;
        case VarType::UINT16:
            return value >= BigInt("0") && value <= BigInt::MAX_UINT16;
        case VarType::UINT24:
            return value >= BigInt("0") && value <= BigInt::MAX_UINT24;
        case VarType::UINT32:
            return value >= BigInt("0") && value <= BigInt::MAX_UINT32;
        case VarType::UINT48:
            return value >= BigInt("0") && value <= BigInt::MAX_UINT48;
        case VarType::UINT64:
            return value >= BigInt("0") && value <= BigInt::MAX_UINT64;
        case VarType::UINT0:
            return value == BigInt("0");
        case VarType::STRING:
            return true;
        default:
            return false;
    }
}

std::string TypeBounds::getTypeRange(VarType type) {
    switch (type) {
        case VarType::BOOL:
            return "true or false";
        case VarType::INT4:
            return BigInt::MIN_INT4.toString() + " to " + BigInt::MAX_INT4.toString();
        case VarType::INT8:
            return BigInt::MIN_INT8.toString() + " to " + BigInt::MAX_INT8.toString();
        case VarType::INT12:
            return BigInt::MIN_INT12.toString() + " to " + BigInt::MAX_INT12.toString();
        case VarType::INT16:
            return BigInt::MIN_INT16.toString() + " to " + BigInt::MAX_INT16.toString();
        case VarType::INT24:
            return BigInt::MIN_INT24.toString() + " to " + BigInt::MAX_INT24.toString();
        case VarType::INT32:
            return BigInt::MIN_INT32.toString() + " to " + BigInt::MAX_INT32.toString();
        case VarType::INT48:
            return BigInt::MIN_INT48.toString() + " to " + BigInt::MAX_INT48.toString();
        case VarType::INT64:
            return BigInt::MIN_INT64.toString() + " to " + BigInt::MAX_INT64.toString();
        case VarType::UINT4:
            return "0 to " + BigInt::MAX_UINT4.toString();
        case VarType::UINT8:
            return "0 to " + BigInt::MAX_UINT8.toString();
        case VarType::UINT12:
            return "0 to " + BigInt::MAX_UINT12.toString();
        case VarType::UINT16:
            return "0 to " + BigInt::MAX_UINT16.toString();
        case VarType::UINT24:
            return "0 to " + BigInt::MAX_UINT24.toString();
        case VarType::UINT32:
            return "0 to " + BigInt::MAX_UINT32.toString();
        case VarType::UINT48:
            return "0 to " + BigInt::MAX_UINT48.toString();
        case VarType::UINT64:
            return "0 to " + BigInt::MAX_UINT64.toString();
        case VarType::UINT0:
            return "0 to 0";
        case VarType::STRING:
            return "string (no numeric bounds)";
        default:
            return "unknown range";
    }
}

std::string TypeBounds::getTypeName(VarType type) {
    switch (type) {
        case VarType::BOOL: return "bool";
        case VarType::INT4: return "int4";
        case VarType::INT8: return "int8";
        case VarType::INT12: return "int12";
        case VarType::INT16: return "int16";
        case VarType::INT24: return "int24";
        case VarType::INT32: return "int32";
        case VarType::INT48: return "int48";
        case VarType::INT64: return "int64";
        case VarType::UINT4: return "uint4";
        case VarType::UINT8: return "uint8";
        case VarType::UINT12: return "uint12";
        case VarType::UINT16: return "uint16";
        case VarType::UINT24: return "uint24";
        case VarType::UINT32: return "uint32";
        case VarType::UINT48: return "uint48";
        case VarType::UINT64: return "uint64";
        case VarType::UINT0: return "uint0";
        case VarType::STRING: return "str";
        case VarType::VOID: return "void";
        default: return "unknown";
    }
}