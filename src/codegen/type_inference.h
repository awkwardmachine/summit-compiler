#pragma once
#include "codegen.h"
#include "ast/ast.h"

namespace AST {
    VarType inferSourceType(llvm::Value* value, CodeGen& context);
    bool isConvertibleToString(llvm::Value* value);
}