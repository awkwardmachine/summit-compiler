#pragma once
#include <llvm/IR/Value.h>
#include <vector>
#include <memory>
#include "ast/ast.h"
#include "codegen/codegen.h"

class FunctionInterface {
public:
    virtual ~FunctionInterface() = default;
    virtual bool handlesCall(const std::string& functionName, size_t argCount) = 0;
    virtual llvm::Value* generateCall(CodeGen& context, AST::CallExpr& expr) = 0;
    virtual std::string getName() const = 0;

    static llvm::Function* ensureExternalFunction(CodeGen& context, const std::string& name, 
        llvm::FunctionType* funcType);
};

using FunctionPtr = std::unique_ptr<FunctionInterface>;