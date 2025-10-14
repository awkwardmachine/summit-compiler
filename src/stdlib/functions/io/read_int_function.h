#pragma once
#include "stdlib/core/function_interface.h"
#include "ast/ast.h"

class ReadIntFunction : public FunctionInterface {
public:
    bool handlesCall(const std::string& functionName, size_t argCount) override;
    llvm::Value* generateCall(CodeGen& context, AST::CallExpr& expr) override;
    std::string getName() const override { return "read_int"; }
    
private:
    llvm::Value* createBoundsCheckCall(CodeGen& context, llvm::Value* value, const std::string& typeName);
    void createBoundsError(CodeGen& context, llvm::Value* value, const std::string& typeName);
};