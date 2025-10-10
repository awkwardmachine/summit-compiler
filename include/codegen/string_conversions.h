#pragma once
#include "codegen.h"

namespace AST {
    llvm::Value* convertToBinaryString(CodeGen& context, llvm::Value* value);
    llvm::Value* convertToDecimalString(CodeGen& context, llvm::Value* value);
    llvm::Value* convertToString(CodeGen& context, llvm::Value* value);
}