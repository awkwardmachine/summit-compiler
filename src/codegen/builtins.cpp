#include "builtins.h"
#include "codegen.h"
#include "ast/ast.h"

using namespace llvm;

void Builtins::createPrintfFunction(CodeGen& context) {
    if (context.getModule().getFunction("printf")) {
        return;
    }
    
    std::vector<Type*> printfArgs;
    printfArgs.push_back(llvm::PointerType::get(Type::getInt8Ty(context.getContext()), 0));
    
    auto printfType = FunctionType::get(
        Type::getInt32Ty(context.getContext()),
        printfArgs,
        true
    );
    Function::Create(printfType, Function::ExternalLinkage, "printf", &context.getModule());
}

void Builtins::createPrintlnFunction(CodeGen& context) {
    createPrintfFunction(context);
}