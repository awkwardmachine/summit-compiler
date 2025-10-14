#pragma once
#include "utils/bigint.h"
#include "ast/ast_types.h"
#include <optional>
#include <utility>

namespace AST {
    class TypeBounds {
    public:
        static bool checkBounds(VarType type, const BigInt& value);
        static std::string getTypeRange(VarType type);
        static std::string getTypeName(VarType type);

        static bool isCastValid(VarType fromType, VarType toType);
        static bool isNumericType(VarType type);
        static bool isIntegerType(VarType type);
        static bool isFloatType(VarType type);
        static size_t getTypeBitWidth(VarType type);
        static bool requiresBoundsCheck(VarType fromType, VarType toType);
        static bool isUnsignedType(VarType type);
        
        static VarType stringToType(const std::string& typeName);
        static std::optional<std::pair<int64_t, int64_t>> getBounds(VarType type);
    };
}