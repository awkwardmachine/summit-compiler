#include "read_int_function.h"
#include "ast/ast.h"
#include "codegen/string_conversions.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>

bool ReadIntFunction::handlesCall(const std::string& functionName, size_t argCount) {
    return functionName == "read_int" && argCount == 0;
}

llvm::Value* ReadIntFunction::generateCall(CodeGen& context, AST::CallExpr& expr) {
    auto& builder = context.getBuilder();
    auto& llvmContext = context.getContext();

    auto funcType = llvm::FunctionType::get(llvm::Type::getInt64Ty(llvmContext), false);
    auto readIntFunc = FunctionInterface::ensureExternalFunction(context, "io_read_int", funcType);

    return builder.CreateCall(readIntFunc, {});
}

llvm::Value* ReadIntFunction::createBoundsCheckCall(CodeGen& context, llvm::Value* value, const std::string& typeName) {
    auto& builder = context.getBuilder();
    auto& llvmContext = context.getContext();
    
    std::string boundsFuncName = "io_check_" + typeName + "_bounds";
    auto boundsFuncType = llvm::FunctionType::get(llvm::Type::getInt1Ty(llvmContext), {llvm::Type::getInt64Ty(llvmContext)}, false);
    auto boundsFunc = FunctionInterface::ensureExternalFunction(context, boundsFuncName, boundsFuncType);
    
    return builder.CreateCall(boundsFunc, {value});
}

void ReadIntFunction::createBoundsError(CodeGen& context, llvm::Value* value, const std::string& typeName) {
    auto& builder = context.getBuilder();
    auto& llvmContext = context.getContext();
    auto& module = context.getModule();
    
    llvm::Function* currentFunc = builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* errorBlock = llvm::BasicBlock::Create(llvmContext, "bounds_error", currentFunc);
    llvm::BasicBlock* continueBlock = llvm::BasicBlock::Create(llvmContext, "bounds_ok", currentFunc);

    llvm::Value* isInBounds = createBoundsCheckCall(context, value, typeName);
    builder.CreateCondBr(isInBounds, continueBlock, errorBlock);

    builder.SetInsertPoint(errorBlock);
    {
        std::string errorMsg = "Error: value %lld out of bounds for " + typeName + "\n";
        llvm::Value* errorStr = builder.CreateGlobalStringPtr(errorMsg);
        
        auto* i8Ptr = llvm::PointerType::get(llvm::Type::getInt8Ty(llvmContext), 0);
        auto fprintfType = llvm::FunctionType::get(llvm::Type::getInt32Ty(llvmContext), {i8Ptr, i8Ptr}, true);
        auto fprintfFunc = FunctionInterface::ensureExternalFunction(context, "fprintf", fprintfType);
        
        llvm::GlobalVariable* stderrVar = module.getNamedGlobal("stderr");
        if (!stderrVar) {
            stderrVar = new llvm::GlobalVariable(module, i8Ptr, false, 
                                                llvm::GlobalValue::ExternalLinkage,
                                                nullptr, "stderr");
        }
        llvm::Value* stderrVal = builder.CreateLoad(i8Ptr, stderrVar);
        
        builder.CreateCall(fprintfFunc, {stderrVal, errorStr, value});
        
        auto exitType = llvm::FunctionType::get(llvm::Type::getVoidTy(llvmContext), {llvm::Type::getInt32Ty(llvmContext)}, false);
        auto exitFunc = FunctionInterface::ensureExternalFunction(context, "exit", exitType);
        builder.CreateCall(exitFunc, {llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvmContext), 1)});
        builder.CreateUnreachable();
    }

    builder.SetInsertPoint(continueBlock);
}