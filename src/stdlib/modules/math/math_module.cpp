#include "math_module.h"
#include <llvm/IR/Function.h>

bool MathModule::handlesModule(const std::string& moduleName) {
    return moduleName == "math";
}

llvm::Value* MathModule::getMember(CodeGen& context, const std::string& moduleName, const std::string& member) {
    if (member == "abs") return createAbsFunction(context);
    if (member == "pow") return createPowFunction(context);
    if (member == "sqrt") return createSqrtFunction(context);
    if (member == "round") return createRoundFunction(context);
    if (member == "min") return createMinFunction(context);
    if (member == "max") return createMaxFunction(context);
    
    throw std::runtime_error("Unknown math function: " + member);
}

llvm::Value* MathModule::createAbsFunction(CodeGen& context) {
    auto& module = context.getModule();
    auto func = module.getFunction("math_abs");
    if (!func) {
        auto& llvmContext = context.getContext();
        auto funcType = llvm::FunctionType::get(
            llvm::Type::getInt32Ty(llvmContext),
            {llvm::Type::getInt32Ty(llvmContext)},
            false
        );
        func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "math_abs", &module);
    }
    return func;
}
llvm::Value* MathModule::createPowFunction(CodeGen& context) {
    auto& module = context.getModule();
    auto func = module.getFunction("math_pow");
    if (!func) {
        auto& llvmContext = context.getContext();
        auto funcType = llvm::FunctionType::get(
            llvm::Type::getFloatTy(llvmContext),
            {llvm::Type::getFloatTy(llvmContext), llvm::Type::getFloatTy(llvmContext)},
            false
        );
        func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "math_pow", &module);
    }
    return func;
}


llvm::Value* MathModule::createSqrtFunction(CodeGen& context) {
    auto& module = context.getModule();
    auto func = module.getFunction("math_sqrt");
    if (!func) {
        auto& llvmContext = context.getContext();
        auto funcType = llvm::FunctionType::get(
            llvm::Type::getFloatTy(llvmContext),
            {llvm::Type::getFloatTy(llvmContext)},
            false
        );
        func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "math_sqrt", &module);
    }
    return func;
}

llvm::Value* MathModule::createRoundFunction(CodeGen& context) {
    auto& module = context.getModule();
    auto func = module.getFunction("math_round");
    if (!func) {
        auto& llvmContext = context.getContext();
        auto funcType = llvm::FunctionType::get(
            llvm::Type::getFloatTy(llvmContext),
            {llvm::Type::getFloatTy(llvmContext)},
            false
        );
        func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "math_round", &module);
    }
    return func;
}

llvm::Value* MathModule::createMinFunction(CodeGen& context) {
    auto& module = context.getModule();
    auto func = module.getFunction("math_min");
    if (!func) {
        auto& llvmContext = context.getContext();
        auto funcType = llvm::FunctionType::get(
            llvm::Type::getFloatTy(llvmContext),
            {llvm::Type::getFloatTy(llvmContext), llvm::Type::getFloatTy(llvmContext)},
            false
        );
        func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "math_min", &module);
    }
    return func;
}

llvm::Value* MathModule::createMaxFunction(CodeGen& context) {
    auto& module = context.getModule();
    auto func = module.getFunction("math_max");
    if (!func) {
        auto& llvmContext = context.getContext();
        auto funcType = llvm::FunctionType::get(
            llvm::Type::getFloatTy(llvmContext),
            {llvm::Type::getFloatTy(llvmContext), llvm::Type::getFloatTy(llvmContext)},
            false
        );
        func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "math_max", &module);
    }
    return func;
}