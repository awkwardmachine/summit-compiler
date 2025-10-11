#include "stdlib.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/IRBuilder.h>
#include <vector>
#include <sstream>

using namespace llvm;

void StandardLibrary::initialize(CodeGen& context) {
    createStdModule(context);
    createIOModule(context);
    createPrintlnFunction(context);
    createStringConversionFunctions(context);
}

void StandardLibrary::createStdModule(CodeGen& context) {
    auto& module = context.getModule();
    auto& llvmContext = context.getContext();
    
    auto* stdType = StructType::create(llvmContext, "std_module_t");

    auto* zeroInitializer = ConstantStruct::get(stdType, {});

    auto stdModule = new GlobalVariable(
        module,
        stdType,
        true,
        GlobalValue::ExternalLinkage,
        zeroInitializer,
        "std"
    );
    
    context.getNamedValues()["std"] = stdModule;
    context.getVariableTypes()["std"] = AST::VarType::MODULE;
}

void StandardLibrary::createIOModule(CodeGen& context) {
    auto& module = context.getModule();
    auto& llvmContext = context.getContext();
    
    auto* ioType = StructType::create(llvmContext, "io_module_t");

    auto* zeroInitializer = ConstantStruct::get(ioType, {});

    auto ioModule = new GlobalVariable(
        module,
        ioType,
        true,
        GlobalValue::ExternalLinkage,
        zeroInitializer,
        "io"
    );
    
    context.getNamedValues()["io"] = ioModule;
    context.getVariableTypes()["io"] = AST::VarType::MODULE;
}

void StandardLibrary::createPrintlnFunction(CodeGen& context) {
    auto& module = context.getModule();
    auto& llvmContext = context.getContext();
    
    if (module.getFunction("io_println_str")) {
        return;
    }
    
    auto* i8Ptr = PointerType::get(Type::getInt8Ty(llvmContext), 0);
    auto* voidType = Type::getVoidTy(llvmContext);

    std::vector<Type*> paramTypes = {i8Ptr};
    auto funcType = FunctionType::get(voidType, paramTypes, false);
    
    auto printlnFunc = Function::Create(
        funcType,
        Function::ExternalLinkage,
        "io_println_str",
        &module
    );
    
    printlnFunc->arg_begin()->setName("str");
    
    auto entryBlock = BasicBlock::Create(llvmContext, "entry", printlnFunc);
    auto& builder = context.getBuilder();
    
    auto savedBlock = builder.GetInsertBlock();
    builder.SetInsertPoint(entryBlock);
    
    auto printfFunc = module.getFunction("printf");
    if (!printfFunc) {
        auto* i32 = Type::getInt32Ty(llvmContext);
        auto printfType = FunctionType::get(i32, {i8Ptr}, true);
        printfFunc = Function::Create(printfType, Function::ExternalLinkage, "printf", &module);
    }
    
    auto formatStr = builder.CreateGlobalStringPtr("%s\n");
    
    auto argIt = printlnFunc->arg_begin();
    Value* strArg = &*argIt;
    
    builder.CreateCall(printfFunc, {formatStr, strArg});
    builder.CreateRetVoid();
    
    if (savedBlock) {
        builder.SetInsertPoint(savedBlock);
    }
}

void StandardLibrary::createStringConversionFunctions(CodeGen& context) {
    auto& module = context.getModule();
    auto& llvmContext = context.getContext();
    
    auto* i8Ptr = PointerType::get(Type::getInt8Ty(llvmContext), 0);
    
    auto* i32 = Type::getInt32Ty(llvmContext);
    std::vector<Type*> sprintfParams = {i8Ptr, i8Ptr};
    auto sprintfType = FunctionType::get(i32, sprintfParams, true);
    if (!module.getFunction("sprintf")) {
        Function::Create(sprintfType, Function::ExternalLinkage, "sprintf", &module);
    }

    auto* sizeT = Type::getInt64Ty(llvmContext);
    auto mallocType = FunctionType::get(i8Ptr, {sizeT}, false);
    if (!module.getFunction("malloc")) {
        Function::Create(mallocType, Function::ExternalLinkage, "malloc", &module);
    }
    
    std::vector<Type*> snprintfParams = {i8Ptr, sizeT, i8Ptr};
    auto snprintfType = FunctionType::get(i32, snprintfParams, true);
    if (!module.getFunction("snprintf")) {
        Function::Create(snprintfType, Function::ExternalLinkage, "snprintf", &module);
    }

    createIntToStringFunction(context, "int8_to_string", Type::getInt8Ty(llvmContext), "%d");
    createIntToStringFunction(context, "int16_to_string", Type::getInt16Ty(llvmContext), "%d");
    createIntToStringFunction(context, "int32_to_string", Type::getInt32Ty(llvmContext), "%d");
    createIntToStringFunction(context, "int64_to_string", Type::getInt64Ty(llvmContext), "%lld");

    createFloatToStringFunction(context, "float_to_string", Type::getFloatTy(llvmContext), "%f");
    createFloatToStringFunction(context, "double_to_string", Type::getDoubleTy(llvmContext), "%lf");

    createBoolToStringFunction(context, "bool_to_string", Type::getInt1Ty(llvmContext));
}

void StandardLibrary::createIntToStringFunction(CodeGen& context, const std::string& name, 
                                               Type* intType, const char* format) {
    auto& module = context.getModule();
    auto& llvmContext = context.getContext();
    
    auto* i8Ptr = PointerType::get(Type::getInt8Ty(llvmContext), 0);
    
    std::vector<Type*> paramTypes = {intType};
    auto funcType = FunctionType::get(i8Ptr, paramTypes, false);
    
    auto func = Function::Create(
        funcType,
        Function::ExternalLinkage,
        name,
        &module
    );
    
    auto entryBlock = BasicBlock::Create(llvmContext, "entry", func);
    auto& builder = context.getBuilder();
    auto savedBlock = builder.GetInsertBlock();
    builder.SetInsertPoint(entryBlock);

    auto mallocFunc = module.getFunction("malloc");
    auto snprintfFunc = module.getFunction("snprintf");
    
    auto bufferSize = ConstantInt::get(Type::getInt64Ty(llvmContext), 32);
    auto buffer = builder.CreateCall(mallocFunc, {bufferSize});
    
    auto formatStr = builder.CreateGlobalStringPtr(format);

    auto argIt = func->arg_begin();
    Value* intArg = &*argIt;

    if (intType->getIntegerBitWidth() < 32) {
        intArg = builder.CreateSExt(intArg, Type::getInt32Ty(llvmContext));
    }
    
    builder.CreateCall(snprintfFunc, {buffer, bufferSize, formatStr, intArg});
    
    builder.CreateRet(buffer);
    
    if (savedBlock) {
        builder.SetInsertPoint(savedBlock);
    }
}

void StandardLibrary::createFloatToStringFunction(CodeGen& context, const std::string& name,
                                                 Type* floatType, const char* format) {
    auto& module = context.getModule();
    auto& llvmContext = context.getContext();
    
    auto* i8Ptr = PointerType::get(Type::getInt8Ty(llvmContext), 0);
    
    std::vector<Type*> paramTypes = {floatType};
    auto funcType = FunctionType::get(i8Ptr, paramTypes, false);
    
    auto func = Function::Create(
        funcType,
        Function::ExternalLinkage,
        name,
        &module
    );
    
    auto entryBlock = BasicBlock::Create(llvmContext, "entry", func);
    auto& builder = context.getBuilder();
    auto savedBlock = builder.GetInsertBlock();
    builder.SetInsertPoint(entryBlock);

    auto mallocFunc = module.getFunction("malloc");
    auto snprintfFunc = module.getFunction("snprintf");

    auto bufferSize = ConstantInt::get(Type::getInt64Ty(llvmContext), 32);
    auto buffer = builder.CreateCall(mallocFunc, {bufferSize});

    auto formatStr = builder.CreateGlobalStringPtr(format);

    auto argIt = func->arg_begin();
    Value* floatArg = &*argIt;

    builder.CreateCall(snprintfFunc, {buffer, bufferSize, formatStr, floatArg});
    
    builder.CreateRet(buffer);
    
    if (savedBlock) {
        builder.SetInsertPoint(savedBlock);
    }
}

void StandardLibrary::createBoolToStringFunction(CodeGen& context, const std::string& name,
                                                Type* boolType) {
    auto& module = context.getModule();
    auto& llvmContext = context.getContext();
    
    auto* i8Ptr = PointerType::get(Type::getInt8Ty(llvmContext), 0);
    
    std::vector<Type*> paramTypes = {boolType};
    auto funcType = FunctionType::get(i8Ptr, paramTypes, false);
    
    auto func = Function::Create(
        funcType,
        Function::ExternalLinkage,
        name,
        &module
    );
    
    auto entryBlock = BasicBlock::Create(llvmContext, "entry", func);
    auto& builder = context.getBuilder();
    auto savedBlock = builder.GetInsertBlock();
    builder.SetInsertPoint(entryBlock);
    
    auto mallocFunc = module.getFunction("malloc");
    auto strcpyFunc = module.getFunction("strcpy");
    if (!strcpyFunc) {
        std::vector<Type*> strcpyParams = {i8Ptr, i8Ptr};
        auto strcpyType = FunctionType::get(i8Ptr, strcpyParams, false);
        strcpyFunc = Function::Create(strcpyType, Function::ExternalLinkage, "strcpy", &module);
    }
    
    auto argIt = func->arg_begin();
    Value* boolArg = &*argIt;
    
    auto trueBlock = BasicBlock::Create(llvmContext, "true", func);
    auto falseBlock = BasicBlock::Create(llvmContext, "false", func);
    auto mergeBlock = BasicBlock::Create(llvmContext, "merge", func);
    
    builder.CreateCondBr(boolArg, trueBlock, falseBlock);
    
    builder.SetInsertPoint(trueBlock);
    auto trueStr = builder.CreateGlobalStringPtr("true");
    auto trueBufferSize = ConstantInt::get(Type::getInt64Ty(llvmContext), 6);
    auto trueBuffer = builder.CreateCall(mallocFunc, {trueBufferSize});
    builder.CreateCall(strcpyFunc, {trueBuffer, trueStr});
    builder.CreateBr(mergeBlock);
    
    builder.SetInsertPoint(falseBlock);
    auto falseStr = builder.CreateGlobalStringPtr("false");
    auto falseBufferSize = ConstantInt::get(Type::getInt64Ty(llvmContext), 7);
    auto falseBuffer = builder.CreateCall(mallocFunc, {falseBufferSize});
    builder.CreateCall(strcpyFunc, {falseBuffer, falseStr});
    builder.CreateBr(mergeBlock);

    builder.SetInsertPoint(mergeBlock);
    auto phi = builder.CreatePHI(i8Ptr, 2, "result");
    phi->addIncoming(trueBuffer, trueBlock);
    phi->addIncoming(falseBuffer, falseBlock);
    
    builder.CreateRet(phi);
    
    if (savedBlock) {
        builder.SetInsertPoint(savedBlock);
    }
}

llvm::Function* StandardLibrary::createModuleFunction(
    CodeGen& context, 
    const std::string& name,
    llvm::Type* returnType,
    const std::vector<llvm::Type*>& paramTypes) {
    
    auto& module = context.getModule();
    
    auto funcType = FunctionType::get(returnType, paramTypes, false);
    auto function = Function::Create(
        funcType,
        Function::ExternalLinkage,
        name,
        &module
    );
    
    return function;
}