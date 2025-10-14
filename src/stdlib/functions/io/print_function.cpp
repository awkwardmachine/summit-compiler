#include "print_function.h"
#include "ast/ast.h"
#include "codegen/string_conversions.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>

bool PrintFunction::handlesCall(const std::string& functionName, size_t argCount) {
    return functionName == "print" && argCount == 1;
}

llvm::Value* PrintFunction::generateCall(CodeGen& context, AST::CallExpr& expr) {
    auto& builder = context.getBuilder();
    auto& llvmContext = context.getContext();
    
    auto argValue = expr.getArgs()[0]->codegen(context);
    auto stringValue = AST::convertToString(context, argValue);
   
    auto* i8Ptr = llvm::PointerType::get(llvm::Type::getInt8Ty(llvmContext), 0);
    auto* voidType = llvm::Type::getVoidTy(llvmContext);
    std::vector<llvm::Type*> paramTypes = {i8Ptr};
    auto funcType = llvm::FunctionType::get(voidType, paramTypes, false);
    
    auto printFunc = FunctionInterface::ensureExternalFunction(context, "io_print_str", funcType);
   
    return builder.CreateCall(printFunc, {stringValue});
}