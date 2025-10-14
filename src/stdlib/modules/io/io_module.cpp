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

    if (member == "readln") {
        return createReadlnFunction(context);
    }
    
    if (member == "read_int") {
        return createReadIntFunction(context);
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

llvm::Value* IOModule::createReadlnFunction(CodeGen& context) {
    auto& module = context.getModule();
    auto readlnFunc = module.getFunction("io_readln");
    if (!readlnFunc) {
        auto& llvmContext = context.getContext();
        auto* i8Ptr = llvm::PointerType::get(llvm::Type::getInt8Ty(llvmContext), 0);
        auto readlnType = llvm::FunctionType::get(i8Ptr, false);
        readlnFunc = llvm::Function::Create(readlnType, llvm::Function::ExternalLinkage, "io_readln", &module);
    }
    return readlnFunc;
}

llvm::Value* IOModule::createReadIntFunction(CodeGen& context) {
    auto& module = context.getModule();
    auto readIntFunc = module.getFunction("io_read_int");
    if (!readIntFunc) {
        auto& llvmContext = context.getContext();
        auto readIntType = llvm::FunctionType::get(llvm::Type::getInt64Ty(llvmContext), false);
        readIntFunc = llvm::Function::Create(readIntType, llvm::Function::ExternalLinkage, "io_read_int", &module);
    }
    return readIntFunc;
}