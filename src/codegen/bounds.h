#pragma once
#include "utils/bigint.h"

namespace AST {
    enum class VarType;
}

namespace AST {

class TypeBounds {
public:
    static bool checkBounds(VarType type, const BigInt& value);
    static std::string getTypeRange(VarType type);
    static std::string getTypeName(VarType type);
};

}