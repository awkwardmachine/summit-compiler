#pragma once
#include "stdlib/core/module_interface.h"

class IOModule : public ModuleInterface {
public:
    bool handlesModule(const std::string& moduleName) override;
    llvm::Value* getMember(CodeGen& context, const std::string& moduleName, const std::string& member) override;
    std::string getName() const override { return "io"; }
    
private:
    llvm::Value* createPrintlnFunction(CodeGen& context);
    llvm::Value* createPrintFunction(CodeGen& context);
    llvm::Value* createReadlnFunction(CodeGen& context);
};