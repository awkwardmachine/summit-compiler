#include "bounds.h"
#include "ast/ast.h"

/* Using the AST namespace */
using namespace AST;

/* Check if the value is within type bounds */
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
        case VarType::MODULE:
            return true;
        default:
            return false;
    }
}

/* Get the range of a type as a string */
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
        case VarType::MODULE:
            return "module (no numeric bounds)";
        default:
            return "unknown range";
    }
}

/* Get the type name as a string */
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
        case VarType::FLOAT32: return "float32";
        case VarType::FLOAT64: return "float64";
        case VarType::STRING: return "str";
        case VarType::VOID: return "void";
        case VarType::MODULE: return "module";
        default: return "unknown";
    }
}

/* Check if a cast from one type to another is valid */
bool TypeBounds::isCastValid(VarType fromType, VarType toType) {
    if (toType == VarType::STRING) {
        return true;
    }
    
    if (fromType == VarType::STRING && toType != VarType::STRING) {
        return false;
    }

    if (fromType == toType) {
        return true;
    }
    
    if (isNumericType(fromType) && isNumericType(toType)) {
        return true;
    }

    if (fromType == VarType::BOOL && isIntegerType(toType)) {
        return true;
    }
    
    if (isIntegerType(fromType) && toType == VarType::BOOL) {
        return true;
    }
    
    return false;
}

/* Check if the type is numeric */
bool TypeBounds::isNumericType(VarType type) {
    return isIntegerType(type) || isFloatType(type) || type == VarType::BOOL;
}

/* Check if the type is an integer type */
bool TypeBounds::isIntegerType(VarType type) {
    return (type == VarType::INT4 || type == VarType::INT8 || type == VarType::INT12 || 
            type == VarType::INT16 || type == VarType::INT24 || type == VarType::INT32 || 
            type == VarType::INT48 || type == VarType::INT64 ||
            type == VarType::UINT4 || type == VarType::UINT8 || type == VarType::UINT12 || 
            type == VarType::UINT16 || type == VarType::UINT24 || type == VarType::UINT32 || 
            type == VarType::UINT48 || type == VarType::UINT64 || type == VarType::UINT0);
}

/* Check if the type is a floating point type */
bool TypeBounds::isFloatType(VarType type) {
    return (type == VarType::FLOAT32 || type == VarType::FLOAT64);
}

/* Get the bit width of the type */
size_t TypeBounds::getTypeBitWidth(VarType type) {
    switch (type) {
        case VarType::BOOL: return 1;
        case VarType::INT4: case VarType::UINT4: return 4;
        case VarType::INT8: case VarType::UINT8: return 8;
        case VarType::INT12: case VarType::UINT12: return 12;
        case VarType::INT16: case VarType::UINT16: return 16;
        case VarType::INT24: case VarType::UINT24: return 24;
        case VarType::INT32: case VarType::UINT32: return 32;
        case VarType::INT48: case VarType::UINT48: return 48;
        case VarType::INT64: case VarType::UINT64: return 64;
        case VarType::FLOAT32: return 32;
        case VarType::FLOAT64: return 64;
        case VarType::UINT0: return 1;
        default: return 0;
    }
}

/* Check if bounds checking is needed when casting types */
bool TypeBounds::requiresBoundsCheck(VarType fromType, VarType toType) {
    if (fromType == toType) return false;
    if (toType == VarType::STRING || fromType == VarType::STRING) return false;
    if (toType == VarType::BOOL || fromType == VarType::BOOL) return false;
    if (isFloatType(fromType) || isFloatType(toType)) return false;
    
    if (isIntegerType(fromType) && isIntegerType(toType)) {
        size_t fromBits = getTypeBitWidth(fromType);
        size_t toBits = getTypeBitWidth(toType);
        
        if (toBits < fromBits) return true;
        
        bool fromUnsigned = isUnsignedType(fromType);
        bool toSigned = (toType == VarType::INT4 || toType == VarType::INT8 || 
                        toType == VarType::INT12 || toType == VarType::INT16 || 
                        toType == VarType::INT24 || toType == VarType::INT32 || 
                        toType == VarType::INT48 || toType == VarType::INT64);
        
        if (fromUnsigned && toSigned && fromBits == toBits) return true;
    }
    
    return false;
}

/* Check if the type is unsigned */
bool TypeBounds::isUnsignedType(VarType type) {
    return (type == VarType::UINT4 || type == VarType::UINT8 || 
            type == VarType::UINT12 || type == VarType::UINT16 ||
            type == VarType::UINT24 || type == VarType::UINT32 || 
            type == VarType::UINT48 || type == VarType::UINT64 ||
            type == VarType::UINT0);
}
