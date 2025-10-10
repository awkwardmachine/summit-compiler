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

    bool isConvertibleToString(llvm::Value* value) {
        return value->getType()->isIntegerTy() || 
               value->getType()->isIntegerTy(1);
    }
}