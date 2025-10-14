#pragma once
#include "stdlib/core/module_interface.h"

class StdModule : public ModuleInterface {
public:
    bool handlesModule(const std::string& moduleName) override;
    llvm::Value* getMember(CodeGen& context, const std::string& moduleName, const std::string& member) override;
    std::string getName() const override { return "std"; }
    
private:
    llvm::Value* handleIOMember(CodeGen& context, const std::string& callerModuleName);
};