#include "type_inference.h"
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

namespace AST {
    VarType inferSourceType(llvm::Value* value, CodeGen& context) {
        if (value->getType()->isFloatTy()) return VarType::FLOAT32;
        if (value->getType()->isDoubleTy()) return VarType::FLOAT64;
        if (value->getType()->isIntegerTy(1)) return VarType::BOOL;
        if (value->getType()->isPointerTy()) return VarType::STRING;
        
        if (value->getType()->isIntegerTy()) {
            return VarType::INT64;
        }
        
        return VarType::VOID;
    }

    VarType inferTypeFromValue(llvm::Value* value, CodeGen& context) {
        if (auto* globalVar = llvm::dyn_cast<llvm::GlobalVariable>(value)) {
            std::string name = globalVar->getName().str();
            
            if (name == "std" || name == "io") {
                return VarType::MODULE;
            }
            
            if (globalVar->getValueType()->isStructTy()) {
                if (auto* structType = llvm::dyn_cast<llvm::StructType>(globalVar->getValueType())) {
                    std::string typeName = structType->getName().str();
                    if (typeName.find("module_t") != std::string::npos) {
                        return VarType::MODULE;
                    }
                }
            }
        }
        
        return inferSourceType(value, context);
    }

    bool isConvertibleToString(llvm::Value* value) {
        return value->getType()->isIntegerTy() || 
               value->getType()->isIntegerTy(1);
    }
}