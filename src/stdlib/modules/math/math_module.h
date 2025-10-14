#pragma once
#include "stdlib/core/module_interface.h"

class MathModule : public ModuleInterface {
public:
    bool handlesModule(const std::string& moduleName) override;
    llvm::Value* getMember(CodeGen& context, const std::string& moduleName, const std::string& member) override;
    std::string getName() const override { return "math"; }
    
private:
    llvm::Value* createAbsFunction(CodeGen& context);
    llvm::Value* createPowFunction(CodeGen& context);
    llvm::Value* createSqrtFunction(CodeGen& context);
    llvm::Value* createRoundFunction(CodeGen& context);
    llvm::Value* createMinFunction(CodeGen& context);
    llvm::Value* createMaxFunction(CodeGen& context);
};