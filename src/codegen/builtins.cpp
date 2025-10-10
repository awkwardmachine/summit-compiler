#include "builtins.h"
#include "codegen.h"
#include "ast/ast.h"

/* Using the LLVM namespace */
using namespace llvm;

/* Create a printf function in the LLVM module if it doesn't exist */
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

/* Create a println function, currently just ensures printf exists */
void Builtins::createPrintlnFunction(CodeGen& context) {
    createPrintfFunction(context);
}
