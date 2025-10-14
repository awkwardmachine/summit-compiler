#include "readln_function.h"
#include "ast/ast.h"
#include "codegen/string_conversions.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>

bool ReadlnFunction::handlesCall(const std::string& functionName, size_t argCount) {
    return functionName == "readln" && argCount == 0;  // readln takes 0 arguments
}

llvm::Value* ReadlnFunction::generateCall(CodeGen& context, AST::CallExpr& expr) {
    auto& builder = context.getBuilder();
    auto& llvmContext = context.getContext();
    
    // io_readln takes no arguments and returns char* (string)
    auto* i8Ptr = llvm::PointerType::get(llvm::Type::getInt8Ty(llvmContext), 0);
    auto funcType = llvm::FunctionType::get(i8Ptr, false);  // No parameters, returns char*
    
    auto readlnFunc = FunctionInterface::ensureExternalFunction(context, "io_readln", funcType);
   
    // Call io_readln() - no arguments needed
    return builder.CreateCall(readlnFunc, {});
}