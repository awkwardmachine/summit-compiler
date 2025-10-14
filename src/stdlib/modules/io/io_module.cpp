#include "io_module.h"
#include <llvm/IR/Function.h>

bool IOModule::handlesModule(const std::string& moduleName) {
    return moduleName == "io";
}

llvm::Value* IOModule::getMember(CodeGen& context, const std::string& moduleName, const std::string& member) {
    if (member == "println") {
        return createPrintlnFunction(context);
    }

    if (member == "print") {
        return createPrintFunction(context);
    }
    
    throw std::runtime_error("Unknown member '" + member + "' in module 'io'");
}

llvm::Value* IOModule::createPrintlnFunction(CodeGen& context) {
    auto& module = context.getModule();
    auto printlnFunc = module.getFunction("io_println_str");
    if (!printlnFunc) {
        auto& llvmContext = context.getContext();
        auto* i8Ptr = llvm::PointerType::get(llvm::Type::getInt8Ty(llvmContext), 0);
        auto printlnType = llvm::FunctionType::get(llvm::Type::getVoidTy(llvmContext), {i8Ptr}, false);
        printlnFunc = llvm::Function::Create(printlnType, llvm::Function::ExternalLinkage, "io_println_str", &module);
    }
    return printlnFunc;
}

llvm::Value* IOModule::createPrintFunction(CodeGen& context) {
    auto& module = context.getModule();
    auto printlnFunc = module.getFunction("io_print_str");
    if (!printlnFunc) {
        auto& llvmContext = context.getContext();
        auto* i8Ptr = llvm::PointerType::get(llvm::Type::getInt8Ty(llvmContext), 0);
        auto printlnType = llvm::FunctionType::get(llvm::Type::getVoidTy(llvmContext), {i8Ptr}, false);
        printlnFunc = llvm::Function::Create(printlnType, llvm::Function::ExternalLinkage, "io_print_str", &module);
    }
    return printlnFunc;
}