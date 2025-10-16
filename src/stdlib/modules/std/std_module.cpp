#include "std_module.h"
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Constants.h>

bool StdModule::handlesModule(const std::string& moduleName) {
    return moduleName == "std";
}

llvm::Value* StdModule::getMember(CodeGen& context, const std::string& moduleName, const std::string& member) {
    if (member == "io") {
        return handleIOMember(context, moduleName);
    }

    if (member == "math") {
        return handleMathMember(context, moduleName);
    }
    
    throw std::runtime_error("Unknown member '" + member + "' in module '" + moduleName + "'");
}

llvm::Value* StdModule::handleIOMember(CodeGen& context, const std::string& callerModuleName) {
    std::string ioVarName = callerModuleName + ".io";
    
    auto ioModule = context.lookupVariable(ioVarName);
    if (!ioModule) {
        auto& module = context.getModule();
        auto& llvmContext = context.getContext();
        auto moduleType = llvm::StructType::create(llvmContext, "module_t");
        auto ioVar = new llvm::GlobalVariable(
            module,
            moduleType,
            true,
            llvm::GlobalValue::ExternalLinkage,
            llvm::ConstantAggregateZero::get(moduleType),
            ioVarName
        );
        
        context.getNamedValues()[ioVarName] = ioVar;
        context.getVariableTypes()[ioVarName] = AST::VarType::MODULE;
        context.setModuleReference(ioVarName, ioVar, "io");
        
        std::cout << "DEBUG: Created IO module reference: " << ioVarName << std::endl;
        return ioVar;
    }
    return ioModule;
}

llvm::Value* StdModule::handleMathMember(CodeGen& context, const std::string& callerModuleName) {
    std::string mathVarName = callerModuleName + ".math";
    
    auto mathModule = context.lookupVariable(mathVarName);
    if (!mathModule) {
        auto& module = context.getModule();
        auto& llvmContext = context.getContext();
        auto moduleType = llvm::StructType::create(llvmContext, "module_t");
        auto mathVar = new llvm::GlobalVariable(
            module,
            moduleType,
            true,
            llvm::GlobalValue::ExternalLinkage,
            llvm::ConstantAggregateZero::get(moduleType),
            mathVarName
        );
        
        context.getNamedValues()[mathVarName] = mathVar;
        context.getVariableTypes()[mathVarName] = AST::VarType::MODULE;
        context.setModuleReference(mathVarName, mathVar, "math");
        
        std::cout << "DEBUG: Created Math module reference: " << mathVarName << std::endl;
        return mathVar;
    }
    return mathModule;
}