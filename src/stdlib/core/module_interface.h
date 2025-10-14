#pragma once
#include <llvm/IR/Value.h>
#include <string>
#include <memory>
#include "codegen/codegen.h"

class ModuleInterface {
public:
    virtual ~ModuleInterface() = default;
    virtual bool handlesModule(const std::string& moduleName) = 0;
    virtual llvm::Value* getMember(CodeGen& context, const std::string& moduleName, const std::string& member) = 0;
    virtual std::string getName() const = 0;
};

using ModulePtr = std::unique_ptr<ModuleInterface>;