#include "function_interface.h"

llvm::Function* FunctionInterface::ensureExternalFunction(CodeGen& context, const std::string& name, 
                                                         llvm::FunctionType* funcType) {
    auto& module = context.getModule();
    auto func = module.getFunction(name);
    if (!func) {
        func = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            name,
            &module
        );
    }
    return func;
}