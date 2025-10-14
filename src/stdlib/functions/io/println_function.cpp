#include "println_function.h"
#include "ast/ast.h"
#include "codegen/string_conversions.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>

bool PrintlnFunction::handlesCall(const std::string& functionName, size_t argCount) {
    return functionName == "println" && argCount == 1;
}

llvm::Value* PrintlnFunction::generateCall(CodeGen& context, AST::CallExpr& expr) {
    auto& builder = context.getBuilder();
    auto& llvmContext = context.getContext();
    
    auto argValue = expr.getArgs()[0]->codegen(context);
    auto stringValue = AST::convertToString(context, argValue);
   
    auto* i8Ptr = llvm::PointerType::get(llvm::Type::getInt8Ty(llvmContext), 0);
    auto* voidType = llvm::Type::getVoidTy(llvmContext);
    std::vector<llvm::Type*> paramTypes = {i8Ptr};
    auto funcType = llvm::FunctionType::get(voidType, paramTypes, false);
    
    auto printFunc = FunctionInterface::ensureExternalFunction(context, "io_println_str", funcType);
   
    return builder.CreateCall(printFunc, {stringValue});
}