#pragma once
#include "stdlib/core/function_interface.h"
#include "ast/ast.h"

class PrintFunction : public FunctionInterface {
public:
    bool handlesCall(const std::string& functionName, size_t argCount) override;
    llvm::Value* generateCall(CodeGen& context, AST::CallExpr& expr) override;
    std::string getName() const override { return "print"; }
};