#include "stmt_codegen.h"
#include "codegen.h"
#include "ast/ast.h"
#include "codegen/bounds.h"
#include "type_inference.h"
#include "expr_codegen.h"
#include "bigint.h"

#include <llvm/IR/Verifier.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Type.h>

using namespace llvm;
using namespace AST;

llvm::Constant* createDefaultValue(llvm::Type* type, VarType varType) {
    if (type->isIntegerTy()) {
        if (TypeBounds::isUnsignedType(varType)) {
            return ConstantInt::get(type, 0, false);
        } else {
            return ConstantInt::get(type, 0, true);
        }
    } else if (type->isFloatTy()) {
        return ConstantFP::get(type, 0.0f);
    } else if (type->isDoubleTy()) {
        return ConstantFP::get(type, 0.0);
    } else if (type->isPointerTy()) {
        return ConstantPointerNull::get(static_cast<llvm::PointerType*>(type));
    }
    return nullptr;
}
llvm::Value* StatementCodeGen::codegenVariableDecl(CodeGen& context, VariableDecl& decl) {
    auto& builder = context.getBuilder();
    auto& module = context.getModule();
    auto& llvmContext = context.getContext();
    
    std::string name = decl.getName();
    VarType type = decl.getType();
    bool isConst = decl.getIsConst();
    auto& valueExpr = decl.getValue();
    
    bool isGlobal = !builder.GetInsertBlock();
    
    std::cout << "DEBUG codegenVariableDecl: Processing variable '" << name 
              << "' type=" << static_cast<int>(type)
              << " isGlobal=" << isGlobal 
              << " structName='" << decl.getStructName() << "'" << std::endl;
    
    if (isGlobal) {
        context.registerGlobalVariable(decl.getName());
        std::cout << "DEBUG: Registered global variable in codegen: " << decl.getName() << std::endl;
        llvm::Type* llvmType;
        if (type == VarType::STRUCT) {
            const std::string& structName = decl.getStructName();
            if (structName.empty()) {
                throw std::runtime_error("Struct type requires a struct name for global variable: " + name);
            }
            llvmType = context.getStructType(structName);
        } else {
            llvmType = context.getLLVMType(type);
        }
        
        if (!llvmType) {
            throw std::runtime_error("Unknown type for global variable: " + name);
        }
        
        if (valueExpr) {
            if (auto* numberExpr = dynamic_cast<NumberExpr*>(valueExpr.get())) {
                const BigInt& bigValue = numberExpr->getValue();
                if (!TypeBounds::checkBounds(type, bigValue)) {
                    throw std::runtime_error(
                        "Value " + bigValue.toString() + " out of bounds for type " + 
                        TypeBounds::getTypeName(type) + ". " +
                        "Valid range: " + TypeBounds::getTypeRange(type)
                    );
                }
            }
            else if (auto* floatExpr = dynamic_cast<FloatExpr*>(valueExpr.get())) {
                if (TypeBounds::isIntegerType(type)) {
                    double floatValue = floatExpr->getValue();
                    BigInt bigValue(static_cast<int64_t>(floatValue));
                    if (!TypeBounds::checkBounds(type, bigValue)) {
                        throw std::runtime_error(
                            "Float value " + std::to_string(floatValue) + " out of bounds for type " + 
                            TypeBounds::getTypeName(type) + ". " +
                            "Valid range: " + TypeBounds::getTypeRange(type)
                        );
                    }
                }
            }
            
            if (auto* moduleExpr = dynamic_cast<ModuleExpr*>(valueExpr.get())) {
                auto moduleValue = ExpressionCodeGen::codegenModule(context, *moduleExpr);
                if (moduleValue) {
                    auto globalVar = new llvm::GlobalVariable(
                        module,
                        moduleValue->getType(),
                        isConst,
                        llvm::GlobalValue::InternalLinkage,
                        llvm::cast<llvm::Constant>(moduleValue),
                        name
                    );
                    context.getNamedValues()[name] = globalVar;
                    context.getVariableTypes()[name] = VarType::MODULE;
                    context.setModuleReference(name, globalVar, moduleExpr->getModuleName());
                    return globalVar;
                }
            }
            else if (auto* memberAccess = dynamic_cast<MemberAccessExpr*>(valueExpr.get())) {
                auto object = memberAccess->getObject().get();
                auto member = memberAccess->getMember();
                
                if (auto* varExpr = dynamic_cast<VariableExpr*>(object)) {
                    auto baseVar = context.lookupVariable(varExpr->getName());
                    if (baseVar) {
                        auto baseType = context.lookupVariableType(varExpr->getName());
                        if (baseType == VarType::MODULE) {
                            std::string moduleName = context.getModuleIdentity(varExpr->getName());
                            if (!moduleName.empty()) {
                                auto placeholderType = llvm::StructType::create(llvmContext, "module_member");
                                auto globalVar = new llvm::GlobalVariable(
                                    module,
                                    placeholderType,
                                    isConst,
                                    llvm::GlobalValue::InternalLinkage,
                                    llvm::ConstantAggregateZero::get(placeholderType),
                                    name
                                );
                                context.getNamedValues()[name] = globalVar;
                                context.getVariableTypes()[name] = VarType::MODULE;
                                context.setModuleReference(name, globalVar, moduleName + "." + member);
                                return globalVar;
                            }
                        }
                    }
                }
                auto value = valueExpr->codegen(context);
                if (llvm::isa<llvm::Constant>(value)) {
                    auto globalVar = new llvm::GlobalVariable(
                        module,
                        value->getType(),
                        isConst,
                        llvm::GlobalValue::InternalLinkage,
                        llvm::cast<llvm::Constant>(value),
                        name
                    );
                    context.getNamedValues()[name] = globalVar;
                    context.getVariableTypes()[name] = type;
                    return globalVar;
                } else {
                    throw std::runtime_error("Global variables can only be initialized with constant expressions");
                }
            }
            else if (auto* enumValue = dynamic_cast<EnumValueExpr*>(valueExpr.get())) {
                auto enumVal = ExpressionCodeGen::codegenEnumValue(context, *enumValue);
                if (llvm::isa<llvm::Constant>(enumVal)) {
                    auto globalVar = new llvm::GlobalVariable(
                        module,
                        enumVal->getType(),
                        isConst,
                        llvm::GlobalValue::InternalLinkage,
                        llvm::cast<llvm::Constant>(enumVal),
                        name
                    );
                    context.getNamedValues()[name] = globalVar;
                    context.getVariableTypes()[name] = type;
                    return globalVar;
                } else {
                    throw std::runtime_error("Enum value is not constant");
                }
            }
            else if (auto* numberExpr = dynamic_cast<NumberExpr*>(valueExpr.get())) {
                const BigInt& bigValue = numberExpr->getValue();
                if (!TypeBounds::checkBounds(decl.getType(), bigValue)) {
                    throw std::runtime_error(
                        "Value " + bigValue.toString() + " out of bounds for type " + 
                        TypeBounds::getTypeName(decl.getType()) + ". " +
                        "Valid range: " + TypeBounds::getTypeRange(decl.getType())
                    );
                }
                auto value = ExpressionCodeGen::codegenNumber(context, *numberExpr);
                if (llvm::isa<llvm::Constant>(value)) {
                    auto globalVar = new llvm::GlobalVariable(
                        module,
                        value->getType(),
                        isConst,
                        llvm::GlobalValue::InternalLinkage,
                        llvm::cast<llvm::Constant>(value),
                        name
                    );
                    context.getNamedValues()[name] = globalVar;
                    context.getVariableTypes()[name] = type;
                    return globalVar;
                }
            }
            else if (auto* floatExpr = dynamic_cast<FloatExpr*>(valueExpr.get())) {
                auto value = ExpressionCodeGen::codegenFloat(context, *floatExpr);
                if (llvm::isa<llvm::Constant>(value)) {
                    auto globalVar = new llvm::GlobalVariable(
                        module,
                        value->getType(),
                        isConst,
                        llvm::GlobalValue::InternalLinkage,
                        llvm::cast<llvm::Constant>(value),
                        name
                    );
                    context.getNamedValues()[name] = globalVar;
                    context.getVariableTypes()[name] = type;
                    return globalVar;
                }
            }
            else if (auto* stringExpr = dynamic_cast<StringExpr*>(valueExpr.get())) {
                auto value = ExpressionCodeGen::codegenString(context, *stringExpr);
                if (llvm::isa<llvm::Constant>(value)) {
                    auto globalVar = new llvm::GlobalVariable(
                        module,
                        value->getType(),
                        isConst,
                        llvm::GlobalValue::InternalLinkage,
                        llvm::cast<llvm::Constant>(value),
                        name
                    );
                    context.getNamedValues()[name] = globalVar;
                    context.getVariableTypes()[name] = type;
                    return globalVar;
                }
            }
            else if (auto* boolExpr = dynamic_cast<BooleanExpr*>(valueExpr.get())) {
                auto value = ExpressionCodeGen::codegenBoolean(context, *boolExpr);
                if (llvm::isa<llvm::Constant>(value)) {
                    auto globalVar = new llvm::GlobalVariable(
                        module,
                        value->getType(),
                        isConst,
                        llvm::GlobalValue::InternalLinkage,
                        llvm::cast<llvm::Constant>(value),
                        name
                    );
                    context.getNamedValues()[name] = globalVar;
                    context.getVariableTypes()[name] = type;
                    return globalVar;
                }
            }
            else if (auto* structLiteral = dynamic_cast<StructLiteralExpr*>(valueExpr.get())) {
                auto value = ExpressionCodeGen::codegenStructLiteral(context, *structLiteral);
                if (llvm::isa<llvm::Constant>(value)) {
                    auto globalVar = new llvm::GlobalVariable(
                        module,
                        value->getType(),
                        isConst,
                        llvm::GlobalValue::InternalLinkage,
                        llvm::cast<llvm::Constant>(value),
                        name
                    );
                    context.getNamedValues()[name] = globalVar;
                    context.getVariableTypes()[name] = type;
                    return globalVar;
                } else {
                    throw std::runtime_error("Global struct variables can only be initialized with constant expressions");
                }
            }
            else {
                auto value = valueExpr->codegen(context);
                if (llvm::isa<llvm::Constant>(value)) {
                    auto globalVar = new llvm::GlobalVariable(
                        module,
                        value->getType(),
                        isConst,
                        llvm::GlobalValue::InternalLinkage,
                        llvm::cast<llvm::Constant>(value),
                        name
                    );
                    context.getNamedValues()[name] = globalVar;
                    context.getVariableTypes()[name] = type;
                    return globalVar;
                } else {
                    throw std::runtime_error("Global variables can only be initialized with constant expressions");
                }
            }
        } else {
            auto globalVar = new llvm::GlobalVariable(
                module,
                llvmType,
                isConst,
                llvm::GlobalValue::InternalLinkage,
                llvm::Constant::getNullValue(llvmType),
                name
            );
            context.getNamedValues()[name] = globalVar;
            context.getVariableTypes()[name] = type;
            return globalVar;
        }
    } else {
        std::cout << "DEBUG: codegenVariableDecl - LOCAL variable '" << name 
                  << "' type=" << static_cast<int>(type) 
                  << " structName='" << decl.getStructName() << "'" << std::endl;
        
        llvm::Type* llvmType;
        if (type == VarType::STRUCT) {
            const std::string& structName = decl.getStructName();
            std::cout << "DEBUG: Local variable '" << name << "' declared with struct type, structName from decl = '" << structName << "'" << std::endl;
            if (structName.empty()) {
                std::cout << "ERROR: structName is empty!" << std::endl;
                throw std::runtime_error("Struct type requires a struct name for variable: " + name);
            }
            llvmType = context.getStructType(structName);
            if (!llvmType) {
                throw std::runtime_error("Unknown struct type: " + structName);
            }
        } else {
            llvmType = context.getLLVMType(type);
        }
        
        if (!llvmType) {
            throw std::runtime_error("Unknown type for variable: " + name);
        }
        
        llvm::Function* currentFunction = builder.GetInsertBlock()->getParent();
        llvm::IRBuilder<> entryBuilder(&currentFunction->getEntryBlock(), currentFunction->getEntryBlock().begin());
        llvm::AllocaInst* alloca = entryBuilder.CreateAlloca(llvmType, nullptr, name);
        
        if (valueExpr) {
            context.setCurrentTargetType(TypeBounds::getTypeName(type));
            
            auto value = valueExpr->codegen(context);
            
            context.clearCurrentTargetType();
            
            if (type != VarType::STRUCT && TypeBounds::isIntegerType(type) && value->getType()->isIntegerTy()) {
                if (auto* constInt = llvm::dyn_cast<llvm::ConstantInt>(value)) {
                    BigInt bigValue;
                    if (TypeBounds::isUnsignedType(type)) {
                        bigValue = BigInt(constInt->getZExtValue());
                    } else {
                        bigValue = BigInt(constInt->getSExtValue());
                    }
                    
                    if (!TypeBounds::checkBounds(type, bigValue)) {
                        throw std::runtime_error(
                            "Value " + bigValue.toString() + " out of bounds for type " + 
                            TypeBounds::getTypeName(type) + " '" + name + "'. " +
                            "Valid range: " + TypeBounds::getTypeRange(type)
                        );
                    }
                } else {
                    value = addRuntimeBoundsChecking(context, value, type, name);
                }
            }
            
            if (type != VarType::STRUCT && value->getType() != llvmType) {
                if (llvmType->isIntegerTy() && value->getType()->isIntegerTy()) {
                    unsigned targetBits = llvmType->getIntegerBitWidth();
                    unsigned sourceBits = value->getType()->getIntegerBitWidth();
                    
                    if (sourceBits > targetBits) {
                        value = builder.CreateTrunc(value, llvmType);
                    } else if (sourceBits < targetBits) {
                        if (TypeBounds::isUnsignedType(type)) {
                            value = builder.CreateZExt(value, llvmType);
                        } else {
                            value = builder.CreateSExt(value, llvmType);
                        }
                    }
                } else if (llvmType->isFPOrFPVectorTy() && value->getType()->isIntegerTy()) {
                    if (TypeBounds::isUnsignedType(AST::inferSourceType(value, context))) {
                        value = builder.CreateUIToFP(value, llvmType);
                    } else {
                        value = builder.CreateSIToFP(value, llvmType);
                    }
                } else if (llvmType->isIntegerTy() && value->getType()->isFPOrFPVectorTy()) {
                    if (TypeBounds::isUnsignedType(type)) {
                        value = builder.CreateFPToUI(value, llvmType);
                    } else {
                        value = builder.CreateFPToSI(value, llvmType);
                    }
                } else if (llvmType->isFPOrFPVectorTy() && value->getType()->isFPOrFPVectorTy()) {
                    if (llvmType->isFloatTy() && value->getType()->isDoubleTy()) {
                        value = builder.CreateFPTrunc(value, llvmType);
                    } else if (llvmType->isDoubleTy() && value->getType()->isFloatTy()) {
                        value = builder.CreateFPExt(value, llvmType);
                    }
                }
            }
            
            builder.CreateStore(value, alloca);
        } else {
            builder.CreateStore(llvm::Constant::getNullValue(llvmType), alloca);
        }
        
        context.getNamedValues()[name] = alloca;
        context.getVariableTypes()[name] = type;
        if (isConst) {
            context.getConstVariables().insert(name);
        }
        
        return alloca;
    }
    
    return nullptr;
}
llvm::Value* StatementCodeGen::codegenAssignment(CodeGen& context, AssignmentStmt& stmt) {
    auto var = context.lookupVariable(stmt.getName());
    if (!var) {
        throw std::runtime_error("Unknown variable: " + stmt.getName());
    }

    if (context.isVariableConst(stmt.getName())) {
        throw std::runtime_error("Cannot assign to const variable: " + stmt.getName());
    }

    VarType varType = context.lookupVariableType(stmt.getName());
    if (varType == VarType::VOID) {
        throw std::runtime_error("Unknown variable type for: " + stmt.getName());
    }
    
    if (varType == VarType::UINT0) {
        throw std::runtime_error("Cannot assign to uint0 — value is always 0");
    }

    auto value = stmt.getValue()->codegen(context);
    auto& builder = context.getBuilder();
    
    llvm::Type* expectedType = context.getLLVMType(varType);

    if (TypeBounds::isIntegerType(varType) && value->getType()->isIntegerTy()) {
        if (auto* constInt = llvm::dyn_cast<llvm::ConstantInt>(value)) {
            BigInt bigValue;
            if (TypeBounds::isUnsignedType(varType)) {
                bigValue = BigInt(constInt->getZExtValue());
            } else {
                bigValue = BigInt(constInt->getSExtValue());
            }
            
            if (!TypeBounds::checkBounds(varType, bigValue)) {
                throw std::runtime_error(
                    "Value " + bigValue.toString() + " out of bounds for type " + 
                    TypeBounds::getTypeName(varType) + " '" + stmt.getName() + "'. " +
                    "Valid range: " + TypeBounds::getTypeRange(varType)
                );
            }
        } else {
            value = addRuntimeBoundsChecking(context, value, varType, stmt.getName());
        }
    }
    
    if (value->getType() != expectedType) {
        if (expectedType->isIntegerTy() && value->getType()->isIntegerTy()) {
            unsigned targetBits = expectedType->getIntegerBitWidth();
            unsigned sourceBits = value->getType()->getIntegerBitWidth();
            
            if (sourceBits > targetBits) {
                value = builder.CreateTrunc(value, expectedType);
            } else if (sourceBits < targetBits) {
                if (TypeBounds::isUnsignedType(varType)) {
                    value = builder.CreateZExt(value, expectedType);
                } else {
                    value = builder.CreateSExt(value, expectedType);
                }
            }
        }
        else if (expectedType->isFPOrFPVectorTy() && value->getType()->isIntegerTy()) {
            value = builder.CreateSIToFP(value, expectedType);
        }
        else if (expectedType->isIntegerTy() && value->getType()->isFPOrFPVectorTy()) {
            value = builder.CreateFPToSI(value, expectedType);
        }
        else if (expectedType->isIntegerTy() && value->getType()->isIntegerTy()) {
            if (value->getType()->getIntegerBitWidth() < expectedType->getIntegerBitWidth()) {
                value = builder.CreateSExt(value, expectedType);
            } else {
                value = builder.CreateTrunc(value, expectedType);
            }
        }
    }
    
    if (value->getType() != expectedType) {
        throw std::runtime_error("Type mismatch in assignment to variable: " + stmt.getName());
    }

    builder.CreateStore(value, var);
    return value;
}

llvm::Value* StatementCodeGen::codegenExprStmt(CodeGen& context, ExprStmt& stmt) {
    return stmt.getExpr()->codegen(context);
}
llvm::Value* StatementCodeGen::codegenGlobalVariable(CodeGen& context, VariableDecl& decl) {
    if (decl.getType() == VarType::VOID) {
        throw std::runtime_error("Cannot declare global variable of type 'void'");
    }

    auto& module = context.getModule();
    auto& llvmContext = context.getContext();
    
    context.registerGlobalVariable(decl.getName());
    std::cout << "DEBUG codegenGlobalVariable: Registering global variable '" << decl.getName() << "'" << std::endl;
    
    llvm::Type* varType = nullptr;
    if (decl.getType() == VarType::STRUCT) {
        const std::string& structName = decl.getStructName();
        std::cout << "DEBUG: Global variable '" << decl.getName() << "' has struct type: '" << structName << "'" << std::endl;
        
        if (structName.empty()) {
            throw std::runtime_error("Struct type requires a struct name for global variable: " + decl.getName());
        }
        
        varType = context.getStructType(structName);
        if (!varType) {
            throw std::runtime_error("Unknown struct type: " + structName);
        }
        std::cout << "DEBUG: Found struct type for global variable '" << decl.getName() << "'" << std::endl;
    } else {
        varType = context.getLLVMType(decl.getType());
    }
    
    if (!varType) {
        throw std::runtime_error("Unknown variable type for global: " + decl.getName());
    }
    
    llvm::Constant* initialValue = nullptr;
    
    if (decl.getValue()) {
        if (auto* structLiteral = dynamic_cast<StructLiteralExpr*>(decl.getValue().get())) {
            std::cout << "DEBUG: Global variable '" << decl.getName() << "' initialized with struct literal" << std::endl;
            
            if (decl.getType() != VarType::STRUCT) {
                throw std::runtime_error("Struct literal can only initialize struct variables");
            }

            const std::string& structName = decl.getStructName();
            llvm::StructType* structType = llvm::cast<llvm::StructType>(varType);
            
            std::vector<llvm::Constant*> fieldValues;
            const auto& structFields = context.getStructFields(structName);
            
            for (size_t i = 0; i < structFields.size(); i++) {
                const auto& fieldInfo = structFields[i];
                llvm::Type* fieldType = structType->getElementType(i);
                VarType fieldVarType = fieldInfo.second;
                
                llvm::Constant* fieldValue = createDefaultValue(fieldType, fieldVarType);
                fieldValues.push_back(fieldValue);
            }
            
            for (const auto& field : structLiteral->getFields()) {
                std::string fieldName = field.first;
                int fieldIndex = context.getStructFieldIndex(structName, fieldName);
                
                if (fieldIndex == -1) {
                    throw std::runtime_error("Unknown field '" + fieldName + "' in struct '" + structName + "'");
                }
                
                auto fieldExpr = field.second.get();
                llvm::Constant* fieldConstant = nullptr;
                
                if (auto* numberExpr = dynamic_cast<NumberExpr*>(fieldExpr)) {
                    const BigInt& bigValue = numberExpr->getValue();
                    llvm::Type* fieldType = structType->getElementType(fieldIndex);
                    
                    if (TypeBounds::isUnsignedType(structFields[fieldIndex].second)) {
                        fieldConstant = ConstantInt::get(fieldType, bigValue.toInt64(), false);
                    } else {
                        fieldConstant = ConstantInt::get(fieldType, bigValue.toInt64(), true);
                    }
                }
                else if (auto* floatExpr = dynamic_cast<FloatExpr*>(fieldExpr)) {
                    llvm::Type* fieldType = structType->getElementType(fieldIndex);
                    if (fieldType->isFloatTy()) {
                        fieldConstant = ConstantFP::get(fieldType, (float)floatExpr->getValue());
                    } else if (fieldType->isDoubleTy()) {
                        fieldConstant = ConstantFP::get(fieldType, floatExpr->getValue());
                    }
                }
                else if (auto* boolExpr = dynamic_cast<BooleanExpr*>(fieldExpr)) {
                    llvm::Type* fieldType = structType->getElementType(fieldIndex);
                    fieldConstant = ConstantInt::get(fieldType, boolExpr->getValue() ? 1 : 0);
                }
                
                if (fieldConstant) {
                    fieldValues[fieldIndex] = fieldConstant;
                } else {
                    throw std::runtime_error("Global struct variables can only be initialized with constant expressions");
                }
            }
            
            initialValue = llvm::ConstantStruct::get(structType, fieldValues);
        }
        else if (auto* moduleExpr = dynamic_cast<ModuleExpr*>(decl.getValue().get())) {
            const std::string& moduleName = moduleExpr->getModuleName();
            
            if (moduleName == "std") {
                if (auto* structType = dyn_cast<StructType>(varType)) {
                    initialValue = ConstantStruct::get(structType, {});
                } else {
                    initialValue = ConstantAggregateZero::get(varType);
                }
                
                auto globalVar = new llvm::GlobalVariable(
                    module,
                    varType,
                    decl.getIsConst(),
                    llvm::GlobalValue::ExternalLinkage,
                    initialValue,
                    decl.getName()
                );
                
                context.getNamedValues()[decl.getName()] = globalVar;
                context.getVariableTypes()[decl.getName()] = VarType::MODULE;
                
                context.registerModuleAlias(decl.getName(), "std", globalVar);
                
                std::cout << "DEBUG: Created global module alias: " << decl.getName() << " -> std" << std::endl;
                
                if (decl.getIsConst()) {
                    context.getConstVariables().insert(decl.getName());
                }
                
                return globalVar;
            } else {
                throw std::runtime_error("Unknown module: " + moduleName);
            }
        }
        else if (auto* memberAccess = dynamic_cast<MemberAccessExpr*>(decl.getValue().get())) {
            std::cout << "DEBUG: Global variable initialized with member access: " << decl.getName() << std::endl;

            try {
                auto value = decl.getValue()->codegen(context);
                if (llvm::isa<llvm::Constant>(value)) {
                    std::cout << "DEBUG: Creating global variable with constant member access value" << std::endl;
                    
                    auto globalVar = new llvm::GlobalVariable(
                        module,
                        value->getType(),
                        decl.getIsConst(),
                        llvm::GlobalValue::ExternalLinkage,
                        llvm::cast<llvm::Constant>(value),
                        decl.getName()
                    );
                    
                    context.getNamedValues()[decl.getName()] = globalVar;
                    context.getVariableTypes()[decl.getName()] = VarType::MODULE;
                    
                    if (auto* varExpr = dynamic_cast<VariableExpr*>(memberAccess->getObject().get())) {
                        std::string baseVarName = varExpr->getName();
                        std::string memberName = memberAccess->getMember();
                        
                        std::string actualModuleName = context.resolveModuleAlias(baseVarName);
                        if (!actualModuleName.empty()) {
                            std::string targetModule = actualModuleName + "." + memberName;
                            context.registerModuleAlias(decl.getName(), targetModule, globalVar);
                            std::cout << "DEBUG: Registered module alias for " << decl.getName() 
                                    << " -> " << targetModule << std::endl;
                        }
                    }
                    
                    std::cout << "DEBUG: Created global variable '" << decl.getName() << "' with type MODULE" << std::endl;
                    
                    if (decl.getIsConst()) {
                        context.getConstVariables().insert(decl.getName());
                    }
                    
                    return globalVar;
                } else {
                    std::cout << "DEBUG: Member access is not constant, creating module alias" << std::endl;
                    
                    if (auto* varExpr = dynamic_cast<VariableExpr*>(memberAccess->getObject().get())) {
                        std::string baseVarName = varExpr->getName();
                        std::string memberName = memberAccess->getMember();
                        
                        std::string actualModuleName = context.resolveModuleAlias(baseVarName);
                        if (!actualModuleName.empty()) {
                            auto moduleType = context.getLLVMType(VarType::MODULE);
                            initialValue = ConstantAggregateZero::get(moduleType);
                            
                            auto globalVar = new llvm::GlobalVariable(
                                module,
                                moduleType,
                                decl.getIsConst(),
                                llvm::GlobalValue::ExternalLinkage,
                                initialValue,
                                decl.getName()
                            );
                            
                            context.getNamedValues()[decl.getName()] = globalVar;
                            context.getVariableTypes()[decl.getName()] = VarType::MODULE;
                            
                            context.registerModuleAlias(decl.getName(), memberName, globalVar);
                            
                            std::cout << "DEBUG: Created module member alias: " << decl.getName() 
                                      << " -> " << memberName << " (via " << baseVarName << " -> " 
                                      << actualModuleName << ")" << std::endl;
                            
                            if (decl.getIsConst()) {
                                context.getConstVariables().insert(decl.getName());
                            }
                            
                            return globalVar;
                        }
                        
                        auto baseVar = context.lookupVariable(baseVarName);
                        if (baseVar) {
                            auto baseType = context.lookupVariableType(baseVarName);
                            if (baseType == VarType::MODULE) {
                                std::string actualModuleName = context.getModuleIdentity(baseVarName);
                                if (!actualModuleName.empty()) {
                                    auto moduleType = context.getLLVMType(VarType::MODULE);
                                    initialValue = ConstantAggregateZero::get(moduleType);
                                    
                                    auto globalVar = new llvm::GlobalVariable(
                                        module,
                                        moduleType,
                                        decl.getIsConst(),
                                        llvm::GlobalValue::ExternalLinkage,
                                        initialValue,
                                        decl.getName()
                                    );
                                    
                                    context.getNamedValues()[decl.getName()] = globalVar;
                                    context.getVariableTypes()[decl.getName()] = VarType::MODULE;
                                    
                                    std::string targetModule = actualModuleName + "." + memberName;
                                    context.registerModuleAlias(decl.getName(), targetModule, globalVar);
                                    
                                    std::cout << "DEBUG: Created module member alias: " << decl.getName() 
                                              << " -> " << targetModule << std::endl;
                                    
                                    if (decl.getIsConst()) {
                                        context.getConstVariables().insert(decl.getName());
                                    }
                                    
                                    return globalVar;
                                }
                            }
                        }
                    }
                    
                    throw std::runtime_error("Global variables can only be initialized with constant expressions or valid module members");
                }
            } catch (const std::exception& e) {
                std::cout << "DEBUG: Error generating member access value: " << e.what() << std::endl;
                throw std::runtime_error("Global variables can only be initialized with constant expressions or module members: " + std::string(e.what()));
            }
        }
        else if (auto* enumValue = dynamic_cast<EnumValueExpr*>(decl.getValue().get())) {
            std::string fullEnumName = enumValue->getEnumName() + "." + enumValue->getMemberName();
            
            auto enumVar = context.lookupVariable(fullEnumName);
            if (enumVar && llvm::isa<llvm::GlobalVariable>(enumVar)) {
                auto globalEnumVar = llvm::cast<llvm::GlobalVariable>(enumVar);
                initialValue = globalEnumVar->getInitializer();
                
                std::cout << "DEBUG: Using enum value " << fullEnumName << " for global " << decl.getName() << std::endl;
            } else {
                throw std::runtime_error("Unknown enum value: " + fullEnumName);
            }
        }
        else if (auto numberExpr = dynamic_cast<NumberExpr*>(decl.getValue().get())) {
            const BigInt& bigValue = numberExpr->getValue();
            if (!TypeBounds::checkBounds(decl.getType(), bigValue)) {
                throw std::runtime_error(
                    "Value " + bigValue.toString() + " out of bounds for type " + 
                    TypeBounds::getTypeName(decl.getType()) + ". " +
                    "Valid range: " + TypeBounds::getTypeRange(decl.getType())
                );
            }
            
            if (TypeBounds::isUnsignedType(decl.getType())) {
                try {
                    int64_t signedValue = bigValue.toInt64();
                    if (signedValue < 0) {
                        throw std::runtime_error("Negative value for unsigned type");
                    }
                    initialValue = ConstantInt::get(varType, signedValue, false);
                } catch (const std::exception& e) {
                    throw std::runtime_error("Value " + bigValue.toString() + 
                                           " cannot be represented as unsigned " + 
                                           TypeBounds::getTypeName(decl.getType()));
                }
            } else {
                initialValue = ConstantInt::get(varType, bigValue.toInt64(), true);
            }
        }
        else if (auto stringExpr = dynamic_cast<StringExpr*>(decl.getValue().get())) {
            if (decl.getType() != VarType::STRING) {
                throw std::runtime_error("String literal can only initialize string variables");
            }
            initialValue = llvm::ConstantDataArray::getString(llvmContext, stringExpr->getValue());
            varType = initialValue->getType();
        }
        else if (auto floatExpr = dynamic_cast<FloatExpr*>(decl.getValue().get())) {
            if (decl.getType() == VarType::FLOAT32) {
                initialValue = ConstantFP::get(Type::getFloatTy(llvmContext), (float)floatExpr->getValue());
            } else if (decl.getType() == VarType::FLOAT64) {
                initialValue = ConstantFP::get(Type::getDoubleTy(llvmContext), floatExpr->getValue());
            } else {
                throw std::runtime_error("Float literal can only initialize float variables");
            }
        }
        else if (auto boolExpr = dynamic_cast<BooleanExpr*>(decl.getValue().get())) {
            if (decl.getType() != VarType::UINT0) {
                throw std::runtime_error("Boolean literal can only initialize uint0 variables");
            }
            initialValue = ConstantInt::get(Type::getInt1Ty(llvmContext), boolExpr->getValue() ? 1 : 0);
        }
        else {
            try {
                auto value = decl.getValue()->codegen(context);
                if (llvm::isa<llvm::Constant>(value)) {
                    auto globalVar = new llvm::GlobalVariable(
                        module,
                        value->getType(),
                        decl.getIsConst(),
                        llvm::GlobalValue::ExternalLinkage,
                        llvm::cast<llvm::Constant>(value),
                        decl.getName()
                    );
                    context.getNamedValues()[decl.getName()] = globalVar;
                    context.getVariableTypes()[decl.getName()] = decl.getType();
                    return globalVar;
                } else {
                    throw std::runtime_error("Global variables can only be initialized with constant expressions");
                }
            } catch (const std::exception& e) {
                throw std::runtime_error("Global variables can only be initialized with constant expressions: " + std::string(e.what()));
            }
        }
    } else {
        if (decl.getType() == VarType::STRUCT) {
            const std::string& structName = decl.getStructName();
            llvm::StructType* structType = llvm::cast<llvm::StructType>(varType);
            
            std::vector<llvm::Constant*> fieldValues;
            for (size_t i = 0; i < structType->getNumElements(); i++) {
                llvm::Type* fieldType = structType->getElementType(i);
                llvm::Constant* fieldValue = llvm::Constant::getNullValue(fieldType);
                fieldValues.push_back(fieldValue);
            }
            
            initialValue = llvm::ConstantStruct::get(structType, fieldValues);
        }
        else if (decl.getType() == VarType::FLOAT32) {
            initialValue = ConstantFP::get(Type::getFloatTy(llvmContext), 0.0);
        }
        else if (decl.getType() == VarType::FLOAT64) {
            initialValue = ConstantFP::get(Type::getDoubleTy(llvmContext), 0.0);
        }
        else if (decl.getType() == VarType::UINT0) {
            initialValue = ConstantInt::get(Type::getInt1Ty(llvmContext), 0);
        }
        else if (decl.getType() == VarType::STRING) {
            initialValue = llvm::ConstantDataArray::getString(llvmContext, "");
            varType = initialValue->getType();
        }
        else if (decl.getType() == VarType::MODULE) {
            if (auto* structType = dyn_cast<StructType>(varType)) {
                initialValue = ConstantStruct::get(structType, {});
            } else {
                initialValue = ConstantAggregateZero::get(varType);
            }
        }
        else {
            initialValue = ConstantInt::get(varType, 0);
        }
    }
    
    auto globalVar = new llvm::GlobalVariable(
        module,
        varType,
        decl.getIsConst(),
        llvm::GlobalValue::ExternalLinkage,
        initialValue,
        decl.getName()
    );
    
    context.getNamedValues()[decl.getName()] = globalVar;
    context.getVariableTypes()[decl.getName()] = decl.getType();
    
    if (decl.getIsConst()) {
        context.getConstVariables().insert(decl.getName());
    }
    
    std::cout << "DEBUG: Created global variable '" << decl.getName() << "' with type " 
              << static_cast<int>(decl.getType()) << std::endl;
    
    return globalVar;
}
llvm::Value* StatementCodeGen::codegenProgram(CodeGen& context, Program& program) {
    auto& builder = context.getBuilder();
    auto& module = context.getModule();
    auto& llvmContext = context.getContext();
    
    std::cout << "DEBUG: First pass - generating enum and struct declarations" << std::endl;
    
    for (size_t i = 0; i < program.getStatements().size(); i++) {
        auto& stmt = program.getStatements()[i];
        if (auto* enumDecl = dynamic_cast<EnumDecl*>(stmt.get())) {
            std::cout << "DEBUG: Generating enum: " << enumDecl->getName() << std::endl;
            enumDecl->codegen(context);
        }
        else if (auto* structDecl = dynamic_cast<StructDecl*>(stmt.get())) {
            std::cout << "DEBUG: Generating struct: " << structDecl->getName() << std::endl;
            structDecl->codegen(context);
        }
    }
    
    std::cout << "DEBUG: Second pass - generating global variables" << std::endl;

    for (size_t i = 0; i < program.getStatements().size(); i++) {
        auto& stmt = program.getStatements()[i];
        if (auto* varDecl = dynamic_cast<VariableDecl*>(stmt.get())) {
            std::cout << "DEBUG: Generating global variable: " << varDecl->getName() << std::endl;
            codegenGlobalVariable(context, *varDecl);
        }
    }

    std::cout << "DEBUG: Third pass - generating functions" << std::endl;

    for (size_t i = 0; i < program.getStatements().size(); i++) {
        auto& stmt = program.getStatements()[i];
        if (auto* funcStmt = dynamic_cast<FunctionStmt*>(stmt.get())) {
            std::cout << "DEBUG: Generating function: " << funcStmt->getName() << std::endl;
            funcStmt->codegen(context);
        }
    }
    
    std::cout << "DEBUG: Fourth pass - generating struct method bodies" << std::endl;

    for (size_t i = 0; i < program.getStatements().size(); i++) {
        auto& stmt = program.getStatements()[i];
        if (auto* structDecl = dynamic_cast<StructDecl*>(stmt.get())) {
            std::cout << "DEBUG: Generating method bodies for struct: " << structDecl->getName() << std::endl;
            codegenStructMethodBodies(context, *structDecl);
        }
    }

    if (program.getHasEntryPoint()) {
        const std::string& entryPointName = program.getEntryPointFunction();
        std::cout << "DEBUG: Using entry point function: " << entryPointName << std::endl;
        
        llvm::Function* entryFunc = module.getFunction(entryPointName);
        if (!entryFunc) {
            throw std::runtime_error("Entry point function '" + entryPointName + "' not found in module");
        }
        
        auto returnType = entryFunc->getReturnType();
        bool isValidReturnType = returnType->isIntegerTy(32) || 
                                 returnType->isVoidTy() || 
                                 returnType->isIntegerTy(1);
        
        if (!isValidReturnType) {
            throw std::runtime_error(
                "Entry point function must return int32, void, or uint0"
            );
        }
        
        if (entryFunc->arg_size() != 0) {
            throw std::runtime_error(
                "Entry point function should not take any parameters (got " + 
                std::to_string(entryFunc->arg_size()) + " parameters)"
            );
        }

        if (returnType->isVoidTy() || returnType->isIntegerTy(1)) {
            std::cout << "DEBUG: Creating wrapper for entry point function" << std::endl;
            
            entryFunc->setName("__entry_" + entryPointName);
            
            auto mainFuncType = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(llvmContext), 
                false
            );
            auto mainFunc = llvm::Function::Create(
                mainFuncType,
                llvm::Function::ExternalLinkage,
                "main",
                &module
            );
            
            auto entryBlock = llvm::BasicBlock::Create(llvmContext, "entry", mainFunc);
            builder.SetInsertPoint(entryBlock);
            
            if (returnType->isVoidTy()) {
                builder.CreateCall(entryFunc, {});
                builder.CreateRet(llvm::ConstantInt::get(llvmContext, llvm::APInt(32, 0)));
            } else {
                auto result = builder.CreateCall(entryFunc, {});
                auto extendedResult = builder.CreateZExt(result, llvm::Type::getInt32Ty(llvmContext));
                builder.CreateRet(extendedResult);
            }
            
            if (llvm::verifyFunction(*mainFunc, &llvm::errs())) {
                std::cerr << "Main wrapper function failed verification. Generated IR:" << std::endl;
                mainFunc->print(llvm::errs());
                throw std::runtime_error("Main wrapper function failed verification");
            }
            
            return mainFunc;
        } else {
            entryFunc->setName("main");
            std::cout << "DEBUG: Using entry point function as main directly" << std::endl;
            return entryFunc;
        }
    }
    else {
        std::cout << "DEBUG: No entry point found, checking for main() function" << std::endl;
        llvm::Function* userMainFunc = module.getFunction("main");
        bool hasUserMain = (userMainFunc != nullptr);
        
        if (hasUserMain) {
            std::cout << "DEBUG: Found user-defined main() function" << std::endl;
            
            auto returnType = userMainFunc->getReturnType();
            bool isValidReturnType = returnType->isIntegerTy(32) || 
                                     returnType->isVoidTy() || 
                                     returnType->isIntegerTy(1);
            
            if (!isValidReturnType) {
                throw std::runtime_error(
                    "main() function must return int32, void, or uint0"
                );
            }
            
            if (userMainFunc->arg_size() != 0) {
                throw std::runtime_error(
                    "main() function should not take any parameters (got " + 
                    std::to_string(userMainFunc->arg_size()) + " parameters)"
                );
            }
            
            if (returnType->isVoidTy() || returnType->isIntegerTy(1)) {
                if (returnType->isVoidTy()) {
                    std::cout << "DEBUG: main() returns void, creating wrapper" << std::endl;
                } else {
                    std::cout << "DEBUG: main() returns uint0, creating wrapper" << std::endl;
                }
                
                userMainFunc->setName("__user_main");

                auto mainFuncType = llvm::FunctionType::get(
                    llvm::Type::getInt32Ty(llvmContext), 
                    false
                );
                auto wrapperMain = llvm::Function::Create(
                    mainFuncType,
                    llvm::Function::ExternalLinkage,
                    "main",
                    &module
                );
                
                auto entryBlock = llvm::BasicBlock::Create(llvmContext, "entry", wrapperMain);
                builder.SetInsertPoint(entryBlock);
                
                if (returnType->isVoidTy()) {
                    builder.CreateCall(userMainFunc, {});
                    builder.CreateRet(llvm::ConstantInt::get(llvmContext, llvm::APInt(32, 0)));
                } else {
                    auto result = builder.CreateCall(userMainFunc, {});
                    auto extendedResult = builder.CreateZExt(result, llvm::Type::getInt32Ty(llvmContext));
                    builder.CreateRet(extendedResult);
                }
                
                if (llvm::verifyFunction(*wrapperMain, &llvm::errs())) {
                    std::cerr << "Wrapper main() function failed verification. Generated IR:" << std::endl;
                    wrapperMain->print(llvm::errs());
                    throw std::runtime_error("Wrapper main() function failed verification");
                }
                
                return wrapperMain;
            } else {
                std::cout << "DEBUG: main() returns int32, using as-is" << std::endl;
                return userMainFunc;
            }
        } else {
            std::cout << "DEBUG: No user-defined main() found, generating auto-main" << std::endl;
            
            auto mainFuncType = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(llvmContext), 
                false
            );
            auto autoMain = llvm::Function::Create(
                mainFuncType,
                llvm::Function::ExternalLinkage,
                "main",
                &module
            );
            
            auto entryBlock = llvm::BasicBlock::Create(llvmContext, "entry", autoMain);
            builder.SetInsertPoint(entryBlock);

            for (size_t i = 0; i < program.getStatements().size(); i++) {
                auto& stmt = program.getStatements()[i];

                if (dynamic_cast<FunctionStmt*>(stmt.get())) {
                    continue;
                }
                
                if (dynamic_cast<EntrypointStmt*>(stmt.get())) {
                    continue;
                }
                
                if (dynamic_cast<VariableDecl*>(stmt.get())) {
                    continue;
                }
                
                if (dynamic_cast<StructDecl*>(stmt.get())) {
                    continue;
                }
                
                if (dynamic_cast<EnumDecl*>(stmt.get())) {
                    continue;
                }
                
                if (builder.GetInsertBlock()->getTerminator()) {
                    break;
                }
                
                stmt->codegen(context);
            }
            
            if (!builder.GetInsertBlock()->getTerminator()) {
                builder.CreateRet(llvm::ConstantInt::get(llvmContext, llvm::APInt(32, 0)));
            }
            
            if (llvm::verifyFunction(*autoMain, &llvm::errs())) {
                std::cerr << "Auto-generated main() failed verification. Generated IR:" << std::endl;
                autoMain->print(llvm::errs());
                throw std::runtime_error("Auto-generated main() failed verification");
            }
            
            return autoMain;
        }
    }
}

llvm::Value* StatementCodeGen::codegenFunctionStmt(CodeGen& context, FunctionStmt& stmt) {
    auto& module = context.getModule();
    auto& llvmContext = context.getContext();
    auto& builder = context.getBuilder();
    
    std::vector<Type*> paramTypes;

    bool isMethod = (stmt.getName().find('.') != std::string::npos);
    bool firstParamIsSelf = !stmt.getParameters().empty() && stmt.getParameters()[0].first == "self";
    
    for (const auto& param : stmt.getParameters()) {
        llvm::Type* paramType = nullptr;
        if (param.second == VarType::STRUCT) {
            std::string structName = param.first == "self" && isMethod 
                ? stmt.getName().substr(0, stmt.getName().find('.'))
                : context.getVariableStructName(param.first);
            
            if (!structName.empty()) {
                paramType = llvm::PointerType::get(context.getStructType(structName), 0);
            } else {
                paramType = llvm::PointerType::get(llvm::Type::getInt8Ty(llvmContext), 0);
            }
        } else {
            paramType = context.getLLVMType(param.second);
        }
        if (!paramType) {
            throw std::runtime_error("Unknown parameter type for: " + param.first);
        }
        paramTypes.push_back(paramType);
    }

    llvm::Type* returnType = nullptr;
    if (stmt.getReturnType() == VarType::STRUCT) {
        std::string returnStructName = stmt.getReturnStructName();
        std::cout << "DEBUG: Function '" << stmt.getName() << "' returns struct: '" << returnStructName << "'" << std::endl;
        
        if (returnStructName.empty()) {
            if (stmt.getName().find('.') != std::string::npos) {
                size_t dotPos = stmt.getName().find('.');
                returnStructName = stmt.getName().substr(0, dotPos);
                std::cout << "DEBUG: Inferred struct name from method: " << returnStructName << std::endl;
            }
        }
        
        if (!returnStructName.empty()) {
            returnType = context.getStructType(returnStructName);
            if (!returnType) {
                throw std::runtime_error("Unknown struct return type: " + returnStructName);
            }
        } else {
            throw std::runtime_error("Struct return type requires a struct name for function: " + stmt.getName());
        }
    } else {
        returnType = context.getLLVMType(stmt.getReturnType());
    }
    
    if (!returnType) {
        throw std::runtime_error("Unknown return type for function: " + stmt.getName());
    }
    
    auto funcType = FunctionType::get(returnType, paramTypes, false);
    
    std::string functionName = stmt.getName();
    if (stmt.getIsEntryPoint()) {
        functionName = "main";
    }
    
    auto function = Function::Create(funcType, Function::ExternalLinkage, functionName, &module);
    
    if (stmt.getBody()) {
        BasicBlock* savedInsertBlock = builder.GetInsertBlock();
        
        context.enterScope();

        auto entryBlock = BasicBlock::Create(llvmContext, "entry", function);
        builder.SetInsertPoint(entryBlock);
        
        unsigned idx = 0;
        for (auto& arg : function->args()) {
            const auto& param = stmt.getParameters()[idx];
            arg.setName(param.first);
            
            if (param.first == "self" && isMethod) {
                context.getNamedValues()[param.first] = &arg;
                context.getVariableTypes()[param.first] = param.second;

                if (param.second == VarType::STRUCT) {
                    std::string structName = stmt.getName().substr(0, stmt.getName().find('.'));
                    context.setVariableStructName(param.first, structName);
                    std::cout << "DEBUG: Set 'self' parameter to struct '" << structName << "' without alloca" << std::endl;
                }
            } else {
                auto alloca = builder.CreateAlloca(arg.getType(), nullptr, param.first);
                builder.CreateStore(&arg, alloca);
                
                context.getNamedValues()[param.first] = alloca;
                context.getVariableTypes()[param.first] = param.second;

                if (param.second == VarType::STRUCT) {
                    std::string paramStructName = stmt.getParameterStructName(idx);
                    if (!paramStructName.empty()) {
                        context.setVariableStructName(param.first, paramStructName);
                    }
                }
            }
            idx++;
        }
        
        stmt.getBody()->codegen(context);
        
        auto currentBlock = builder.GetInsertBlock();
        if (!currentBlock->getTerminator()) {
            if (stmt.getReturnType() == VarType::VOID) {
                builder.CreateRetVoid();
            } else {
                if (stmt.getReturnType() == VarType::STRUCT) {
                    throw std::runtime_error("Function '" + stmt.getName() + 
                                           "' with struct return type must return a value on all code paths");
                } else {
                    throw std::runtime_error("Function '" + stmt.getName() + 
                                           "' with return type '" + 
                                           TypeBounds::getTypeName(stmt.getReturnType()) + 
                                           "' must return a value on all code paths");
                }
            }
        }
        
        context.exitScope();
        
        if (verifyFunction(*function, &llvm::errs())) {
            function->print(llvm::errs());
            throw std::runtime_error("Function '" + stmt.getName() + "' failed verification");
        }
        
        if (savedInsertBlock) {
            builder.SetInsertPoint(savedInsertBlock);
        }
    }
    
    return function;
}

llvm::Value* StatementCodeGen::codegenBlockStmt(CodeGen& context, BlockStmt& stmt) {
    auto& builder = context.getBuilder();

    for (auto& stmt : stmt.getStatements()) {
        stmt->codegen(context);
        
        if (builder.GetInsertBlock()->getTerminator()) {
            break;
        }
    }
    
    return nullptr;
}
llvm::Value* StatementCodeGen::codegenIfStmt(CodeGen& context, IfStmt& stmt) {
    auto& builder = context.getBuilder();
    auto& llvmContext = context.getContext();
    
    auto condValue = stmt.getCondition()->codegen(context);
    
    if (!condValue->getType()->isIntegerTy(1)) {
        if (condValue->getType()->isIntegerTy()) {
            condValue = builder.CreateICmpNE(condValue, 
                ConstantInt::get(condValue->getType(), 0));
        } else {
            throw std::runtime_error("If condition must be a boolean or integer type");
        }
    }
    
    auto currentFunction = builder.GetInsertBlock()->getParent();
    
    auto thenBlock = BasicBlock::Create(llvmContext, "then", currentFunction);
    auto mergeBlock = BasicBlock::Create(llvmContext, "ifcont");
    BasicBlock* elseBlock = mergeBlock;
    
    if (stmt.getElseBranch()) {
        elseBlock = BasicBlock::Create(llvmContext, "else");
    }
    
    builder.CreateCondBr(condValue, thenBlock, elseBlock);

    builder.SetInsertPoint(thenBlock);
    stmt.getThenBranch()->codegen(context);
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(mergeBlock);
    }

    if (stmt.getElseBranch()) {
        currentFunction->insert(currentFunction->end(), elseBlock);
        builder.SetInsertPoint(elseBlock);
        stmt.getElseBranch()->codegen(context);
        if (!builder.GetInsertBlock()->getTerminator()) {
            builder.CreateBr(mergeBlock);
        }
    }

    currentFunction->insert(currentFunction->end(), mergeBlock);
    builder.SetInsertPoint(mergeBlock);
    
    return nullptr;
}
llvm::Value* StatementCodeGen::codegenReturnStmt(CodeGen& context, ReturnStmt& stmt) {
    auto& builder = context.getBuilder();
    auto& llvmContext = context.getContext();
    
    auto currentFunction = builder.GetInsertBlock()->getParent();
    if (!currentFunction) {
        throw std::runtime_error("Return statement not in a function");
    }
    
    auto expectedReturnType = currentFunction->getReturnType();
    
    if (stmt.getValue()) {
        auto retValue = stmt.getValue()->codegen(context);
        if (!retValue) {
            throw std::runtime_error("Failed to generate return value");
        }
        
        std::cout << "DEBUG: Return statement - expected type: ";
        expectedReturnType->print(llvm::errs());
        llvm::errs() << ", actual type: ";
        retValue->getType()->print(llvm::errs());
        llvm::errs() << "\n";
        
        if (expectedReturnType->isStructTy()) {
            std::cout << "DEBUG: Handling struct return type\n";
            
            if (retValue->getType() != expectedReturnType) {
                if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(retValue)) {
                    retValue = builder.CreateLoad(expectedReturnType, alloca, "struct_ret_val");
                } else if (retValue->getType()->isPointerTy()) {
                    retValue = builder.CreateLoad(expectedReturnType, retValue, "struct_ret_val");
                } else {
                    std::string expectedTypeStr, actualTypeStr;
                    llvm::raw_string_ostream expectedOS(expectedTypeStr), actualOS(actualTypeStr);
                    expectedReturnType->print(expectedOS);
                    retValue->getType()->print(actualOS);
                    throw std::runtime_error("Return type mismatch: expected struct " + expectedTypeStr + 
                                           ", got " + actualTypeStr);
                }
            }
            builder.CreateRet(retValue);
            return nullptr;
        }

        if (expectedReturnType->isIntegerTy(1)) {
            if (auto* constInt = dyn_cast<ConstantInt>(retValue)) {
                if (!constInt->isZero()) {
                    throw std::runtime_error("uint0 functions can only return 0");
                }
            }
            if (retValue->getType()->isIntegerTy()) {
                if (retValue->getType()->getIntegerBitWidth() != 1) {
                    retValue = builder.CreateTrunc(retValue, expectedReturnType);
                }
            } else {
                retValue = ConstantInt::get(expectedReturnType, 0);
            }
        }
        else if (retValue->getType() != expectedReturnType) {
            if (expectedReturnType->isIntegerTy() && retValue->getType()->isIntegerTy()) {
                unsigned expectedBits = expectedReturnType->getIntegerBitWidth();
                unsigned actualBits = retValue->getType()->getIntegerBitWidth();
                
                if (actualBits > expectedBits) {
                    retValue = builder.CreateTrunc(retValue, expectedReturnType, "truncret");
                } else {
                    VarType sourceType = AST::inferSourceType(retValue, context);
                    if (TypeBounds::isUnsignedType(sourceType)) {
                        retValue = builder.CreateZExt(retValue, expectedReturnType, "zextret");
                    } else {
                        retValue = builder.CreateSExt(retValue, expectedReturnType, "sextret");
                    }
                }
            } 
            else if (expectedReturnType->isFPOrFPVectorTy() && retValue->getType()->isFPOrFPVectorTy()) {
                if (expectedReturnType->isFloatTy() && retValue->getType()->isDoubleTy()) {
                    retValue = builder.CreateFPTrunc(retValue, expectedReturnType, "fptruncret");
                } else if (expectedReturnType->isDoubleTy() && retValue->getType()->isFloatTy()) {
                    retValue = builder.CreateFPExt(retValue, expectedReturnType, "fpextret");
                }
            }
            else if (expectedReturnType->isFPOrFPVectorTy() && retValue->getType()->isIntegerTy()) {
                VarType sourceType = AST::inferSourceType(retValue, context);
                if (TypeBounds::isUnsignedType(sourceType)) {
                    retValue = builder.CreateUIToFP(retValue, expectedReturnType, "uitofpret");
                } else {
                    retValue = builder.CreateSIToFP(retValue, expectedReturnType, "sitofpret");
                }
            }
            else if (expectedReturnType->isIntegerTy() && retValue->getType()->isFPOrFPVectorTy()) {
                retValue = builder.CreateFPToSI(retValue, expectedReturnType, "fptosiret");
            }
            else if (expectedReturnType->isPointerTy() && retValue->getType()->isPointerTy()) {
                
            }
            else if (expectedReturnType->isIntegerTy() && retValue->getType()->isIntegerTy(1)) {
                retValue = builder.CreateZExt(retValue, expectedReturnType, "booltointret");
            }
            else if (expectedReturnType->isIntegerTy(1) && retValue->getType()->isIntegerTy()) {
                retValue = builder.CreateICmpNE(retValue, 
                    ConstantInt::get(retValue->getType(), 0), "inttoboolret");
            }
            else if (expectedReturnType->isVoidTy()) {
                throw std::runtime_error("Cannot return a value from void function");
            }
            else {
                std::string expectedTypeStr;
                llvm::raw_string_ostream expectedOS(expectedTypeStr);
                expectedReturnType->print(expectedOS);
                
                std::string actualTypeStr;
                llvm::raw_string_ostream actualOS(actualTypeStr);
                retValue->getType()->print(actualOS);
                
                throw std::runtime_error("Return type mismatch: expected " + expectedTypeStr + 
                                       ", got " + actualTypeStr);
            }
        }

        if (retValue->getType() != expectedReturnType) {
            std::string expectedTypeStr;
            llvm::raw_string_ostream expectedOS(expectedTypeStr);
            expectedReturnType->print(expectedOS);
            
            std::string actualTypeStr;
            llvm::raw_string_ostream actualOS(actualTypeStr);
            retValue->getType()->print(actualOS);
            
            throw std::runtime_error("Return type conversion failed: expected " + 
                                   expectedTypeStr + ", got " + actualTypeStr);
        }
        
        builder.CreateRet(retValue);
    } else {
        if (expectedReturnType->isStructTy()) {
            throw std::runtime_error("Function with struct return type must return a value");
        } else if (expectedReturnType->isIntegerTy(1)) {
            builder.CreateRet(ConstantInt::get(expectedReturnType, 0));
        } else if (!expectedReturnType->isVoidTy()) {
            throw std::runtime_error("Non-void function must return a value");
        } else {
            builder.CreateRetVoid();
        }
    }
    
    return nullptr;
}

llvm::Value* StatementCodeGen::codegenWhileStmt(CodeGen& context, WhileStmt& stmt) {
    auto& builder = context.getBuilder();
    auto& llvmContext = context.getContext();
    
    auto currentFunction = builder.GetInsertBlock()->getParent();
    
    auto conditionBlock = BasicBlock::Create(llvmContext, "while.condition", currentFunction);
    auto bodyBlock = BasicBlock::Create(llvmContext, "while.body");
    auto afterBlock = BasicBlock::Create(llvmContext, "while.end");
    
    context.pushLoopBlocks(afterBlock, conditionBlock);
    
    builder.CreateBr(conditionBlock);
    
    builder.SetInsertPoint(conditionBlock);
    auto condValue = stmt.getCondition()->codegen(context);
    
    if (!condValue->getType()->isIntegerTy(1)) {
        if (condValue->getType()->isIntegerTy()) {
            condValue = builder.CreateICmpNE(condValue, 
                ConstantInt::get(condValue->getType(), 0));
        } else {
            throw std::runtime_error("While condition must be a boolean or integer type");
        }
    }
    
    builder.CreateCondBr(condValue, bodyBlock, afterBlock);
    
    currentFunction->insert(currentFunction->end(), bodyBlock);
    builder.SetInsertPoint(bodyBlock);
    
    stmt.getBody()->codegen(context);

    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(conditionBlock);
    }
    
    currentFunction->insert(currentFunction->end(), afterBlock);
    builder.SetInsertPoint(afterBlock);
    
    context.popLoopBlocks();
    
    return nullptr;
}

llvm::Value* StatementCodeGen::codegenForLoopStmt(CodeGen& context, ForLoopStmt& stmt) {
    auto& builder = context.getBuilder();
    auto& llvmContext = context.getContext();
   
    auto currentFunction = builder.GetInsertBlock()->getParent();
   
    auto conditionBlock = BasicBlock::Create(llvmContext, "for.condition", currentFunction);
    auto bodyBlock = BasicBlock::Create(llvmContext, "for.body");
    auto incrementBlock = BasicBlock::Create(llvmContext, "for.increment");
    auto afterBlock = BasicBlock::Create(llvmContext, "for.end");

    context.pushLoopBlocks(afterBlock, incrementBlock);

    context.enterScope();

    auto varType = stmt.getVarType();
    auto llvmVarType = context.getLLVMType(varType);
    IRBuilder<> entryBuilder(&currentFunction->getEntryBlock(),
                           currentFunction->getEntryBlock().begin());
    auto alloca = entryBuilder.CreateAlloca(llvmVarType, nullptr, stmt.getVarName());
   
    if (stmt.getInitializer()) {
        auto initValue = stmt.getInitializer()->codegen(context);

        if (TypeBounds::isIntegerType(varType) && initValue->getType()->isIntegerTy()) {
            if (auto* constInt = llvm::dyn_cast<llvm::ConstantInt>(initValue)) {
                BigInt bigValue;
                if (TypeBounds::isUnsignedType(varType)) {
                    bigValue = BigInt(constInt->getZExtValue());
                } else {
                    bigValue = BigInt(constInt->getSExtValue());
                }
                
                if (!TypeBounds::checkBounds(varType, bigValue)) {
                    throw std::runtime_error(
                        "Value " + bigValue.toString() + " out of bounds for type " + 
                        TypeBounds::getTypeName(varType) + " '" + stmt.getVarName() + "'. " +
                        "Valid range: " + TypeBounds::getTypeRange(varType)
                    );
                }
            } else {
                initValue = addRuntimeBoundsChecking(context, initValue, varType, stmt.getVarName());
            }
        }

        if (initValue->getType() != llvmVarType) {
            if (llvmVarType->isIntegerTy() && initValue->getType()->isIntegerTy()) {
                unsigned targetBits = llvmVarType->getIntegerBitWidth();
                unsigned sourceBits = initValue->getType()->getIntegerBitWidth();
                
                if (sourceBits > targetBits) {
                    initValue = builder.CreateTrunc(initValue, llvmVarType);
                } else if (sourceBits < targetBits) {
                    if (TypeBounds::isUnsignedType(varType)) {
                        initValue = builder.CreateZExt(initValue, llvmVarType);
                    } else {
                        initValue = builder.CreateSExt(initValue, llvmVarType);
                    }
                }
            }
        }
        
        builder.CreateStore(initValue, alloca);
    } else {
        auto zero = ConstantInt::get(llvmVarType, 0);
        builder.CreateStore(zero, alloca);
    }

    context.getNamedValues()[stmt.getVarName()] = alloca;
    context.getVariableTypes()[stmt.getVarName()] = stmt.getVarType();
   
    builder.CreateBr(conditionBlock);
   
    builder.SetInsertPoint(conditionBlock);
    
    auto currentVal = builder.CreateLoad(llvmVarType, alloca, stmt.getVarName());
    
    auto condValue = stmt.getCondition()->codegen(context);

    if (!condValue->getType()->isIntegerTy(1)) {
        if (condValue->getType()->isIntegerTy()) {
            condValue = builder.CreateICmpNE(condValue,
                ConstantInt::get(condValue->getType(), 0));
        } else {
            throw std::runtime_error("For loop condition must be a boolean or integer type");
        }
    }
   
    builder.CreateCondBr(condValue, bodyBlock, afterBlock);
   
    currentFunction->insert(currentFunction->end(), bodyBlock);
    builder.SetInsertPoint(bodyBlock);
   
    stmt.getBody()->codegen(context);

    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(incrementBlock);
    }
   
    currentFunction->insert(currentFunction->end(), incrementBlock);
    builder.SetInsertPoint(incrementBlock);
   
    if (stmt.getIncrement()) {
        auto incrementValue = stmt.getIncrement()->codegen(context);
        if (incrementValue) {
            if (TypeBounds::isIntegerType(varType) && incrementValue->getType()->isIntegerTy()) {
                if (auto* constInt = llvm::dyn_cast<llvm::ConstantInt>(incrementValue)) {
                    BigInt bigValue;
                    if (TypeBounds::isUnsignedType(varType)) {
                        bigValue = BigInt(constInt->getZExtValue());
                    } else {
                        bigValue = BigInt(constInt->getSExtValue());
                    }
                    
                    if (!TypeBounds::checkBounds(varType, bigValue)) {
                        throw std::runtime_error(
                            "Increment value " + bigValue.toString() + " out of bounds for type " + 
                            TypeBounds::getTypeName(varType) + " '" + stmt.getVarName() + "'. " +
                            "Valid range: " + TypeBounds::getTypeRange(varType)
                        );
                    }
                } else {
                    incrementValue = addRuntimeBoundsChecking(context, incrementValue, varType, stmt.getVarName() + "_increment");
                }
            }
            
            auto currentVal = builder.CreateLoad(llvmVarType, alloca, stmt.getVarName() + ".load");
            
            if (incrementValue->getType() != llvmVarType) {
                if (llvmVarType->isIntegerTy() && incrementValue->getType()->isIntegerTy()) {
                    unsigned targetBits = llvmVarType->getIntegerBitWidth();
                    unsigned sourceBits = incrementValue->getType()->getIntegerBitWidth();
                    
                    if (sourceBits > targetBits) {
                        incrementValue = builder.CreateTrunc(incrementValue, llvmVarType);
                    } else if (sourceBits < targetBits) {
                        incrementValue = builder.CreateSExt(incrementValue, llvmVarType);
                    }
                }
            }

            builder.CreateStore(incrementValue, alloca);
        }
    }
   
    builder.CreateBr(conditionBlock);
   
    currentFunction->insert(currentFunction->end(), afterBlock);
    builder.SetInsertPoint(afterBlock);
   
    context.exitScope();
    context.popLoopBlocks();
   
    return nullptr;
}

llvm::Value* StatementCodeGen::codegenEnumDecl(CodeGen& context, EnumDecl& decl) {
    auto& module = context.getModule();
    auto& llvmContext = context.getContext();
    
    for (const auto& member : decl.getMembers()) {
        std::string fullName = decl.getName() + "." + member.first;
        
        auto value = member.second->codegen(context);
        
        llvm::Value* intValue = value;
        if (!value->getType()->isIntegerTy()) {
            auto& builder = context.getBuilder();
            if (value->getType()->isFPOrFPVectorTy()) {
                intValue = builder.CreateFPToSI(value, llvm::Type::getInt32Ty(llvmContext));
            } else {
                throw std::runtime_error("Enum values must be integers");
            }
        }
        
        llvm::GlobalVariable* enumValue = new llvm::GlobalVariable(
            module,
            llvm::Type::getInt32Ty(llvmContext),
            true,
            llvm::GlobalValue::InternalLinkage,
            llvm::cast<llvm::Constant>(intValue),
            fullName
        );
        
        context.getNamedValues()[fullName] = enumValue;
        context.getVariableTypes()[fullName] = VarType::INT32;
    }
    
    return nullptr;
}

llvm::Value* StatementCodeGen::codegenBreakStmt(CodeGen& context, BreakStmt& stmt) {
    auto& builder = context.getBuilder();

    llvm::BasicBlock* breakBlock = context.getCurrentLoopExitBlock();
    if (!breakBlock) {
        throw std::runtime_error("Break statement not inside a loop");
    }
    
    builder.CreateBr(breakBlock);

    return nullptr;
}

llvm::Value* StatementCodeGen::codegenContinueStmt(CodeGen& context, ContinueStmt& stmt) {
    auto& builder = context.getBuilder();
    
    llvm::BasicBlock* continueBlock = context.getCurrentLoopContinueBlock();
    if (!continueBlock) {
        throw std::runtime_error("Continue statement not inside a loop");
    }

    builder.CreateBr(continueBlock);
    return nullptr;
}

llvm::Value* StatementCodeGen::addRuntimeBoundsChecking(CodeGen& context, llvm::Value* value, AST::VarType targetType, const std::string& varName) {
    auto& builder = context.getBuilder();
    auto& llvmContext = context.getContext();
    auto& module = context.getModule();
    
    auto bounds = AST::TypeBounds::getBounds(targetType);
    if (!bounds.has_value()) {
        return value;
    }
    
    auto [minVal, maxVal] = bounds.value();
    
    llvm::Value* minBound = llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvmContext), minVal);
    llvm::Value* maxBound = llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvmContext), maxVal);

    llvm::Value* value64;
    if (value->getType()->isIntegerTy()) {
        if (AST::TypeBounds::isUnsignedType(targetType)) {
            value64 = builder.CreateZExt(value, llvm::Type::getInt64Ty(llvmContext));
        } else {
            value64 = builder.CreateSExt(value, llvm::Type::getInt64Ty(llvmContext));
        }
    } else {
        return value;
    }
    
    llvm::Value* isGeMin;
    llvm::Value* isLeMax;
    
    if (AST::TypeBounds::isUnsignedType(targetType)) {
        isGeMin = builder.CreateICmpUGE(value64, minBound, varName + "_bounds_uge_min");
        isLeMax = builder.CreateICmpULE(value64, maxBound, varName + "_bounds_ule_max");
    } else {
        isGeMin = builder.CreateICmpSGE(value64, minBound, varName + "_bounds_sge_min");
        isLeMax = builder.CreateICmpSLE(value64, maxBound, varName + "_bounds_sle_max");
    }
    
    llvm::Value* isInBounds = builder.CreateAnd(isGeMin, isLeMax, varName + "_bounds_check");

    llvm::Function* currentFunc = builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* errorBlock = llvm::BasicBlock::Create(llvmContext, varName + "_bounds_error", currentFunc);
    llvm::BasicBlock* continueBlock = llvm::BasicBlock::Create(llvmContext, varName + "_bounds_ok", currentFunc);

    builder.CreateCondBr(isInBounds, continueBlock, errorBlock);

    builder.SetInsertPoint(errorBlock);
    {
        std::string errorMsg = "Error: value %lld out of bounds for " + 
                              AST::TypeBounds::getTypeName(targetType) + " '" + varName + 
                              "' (must be between " + std::to_string(minVal) + 
                              " and " + std::to_string(maxVal) + ")\n";
        llvm::Value* errorStr = builder.CreateGlobalStringPtr(errorMsg);

        auto* i8Ptr = llvm::PointerType::get(llvm::Type::getInt8Ty(llvmContext), 0);
        auto fprintfType = llvm::FunctionType::get(llvm::Type::getInt32Ty(llvmContext), {i8Ptr, i8Ptr}, true);
        auto fprintfFunc = module.getFunction("fprintf");
        if (!fprintfFunc) {
            fprintfFunc = llvm::Function::Create(fprintfType, llvm::Function::ExternalLinkage, "fprintf", &module);
        }
        
        llvm::GlobalVariable* stderrVar = module.getNamedGlobal("stderr");
        if (!stderrVar) {
            stderrVar = new llvm::GlobalVariable(module, i8Ptr, false, 
                                                llvm::GlobalValue::ExternalLinkage,
                                                nullptr, "stderr");
        }
        llvm::Value* stderrVal = builder.CreateLoad(i8Ptr, stderrVar);

        builder.CreateCall(fprintfFunc, {stderrVal, errorStr, value64});
        
        auto exitType = llvm::FunctionType::get(llvm::Type::getVoidTy(llvmContext), {llvm::Type::getInt32Ty(llvmContext)}, false);
        auto exitFunc = module.getFunction("exit");
        if (!exitFunc) {
            exitFunc = llvm::Function::Create(exitType, llvm::Function::ExternalLinkage, "exit", &module);
        }
        builder.CreateCall(exitFunc, {llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvmContext), 1)});
        builder.CreateUnreachable();
    }
    
    builder.SetInsertPoint(continueBlock);
    
    return value;
}

llvm::Value* StatementCodeGen::codegenStructDecl(CodeGen& context, StructDecl& decl) {
    auto& llvmContext = context.getContext();
    auto& module = context.getModule();
    
    std::string structName = decl.getName();
    std::cout << "DEBUG codegenStructDecl: Generating struct '" << structName << "' with " 
              << decl.getFields().size() << " fields and " << decl.getMethods().size() 
              << " methods" << std::endl;
    
    std::vector<llvm::Type*> fieldTypes;
    for (const auto& field : decl.getFields()) {
        llvm::Type* fieldType;
        if (field.second == AST::VarType::STRUCT) {
            fieldType = context.getStructType(field.first);
            if (!fieldType) {
                throw std::runtime_error("Unknown struct type for field: " + field.first);
            }
        } else {
            fieldType = context.getLLVMType(field.second);
        }
        
        if (!fieldType) {
            throw std::runtime_error("Unknown field type in struct '" + structName + "' for field: " + field.first);
        }
        fieldTypes.push_back(fieldType);
        std::cout << "DEBUG: Field '" << field.first << "' type: " << static_cast<int>(field.second) << std::endl;
    }
    
    llvm::StructType* structType = llvm::StructType::create(llvmContext, fieldTypes, structName);
    
    context.registerStructType(structName, structType, decl.getFields());
    for (const auto& [fieldName, defaultValueExpr] : decl.getFieldDefaults()) {
        if (defaultValueExpr) {
            try {
                if (auto* floatExpr = dynamic_cast<AST::FloatExpr*>(defaultValueExpr.get())) {
                    VarType fieldType = VarType::VOID;
                    for (const auto& field : decl.getFields()) {
                        if (field.first == fieldName) {
                            fieldType = field.second;
                            break;
                        }
                    }
                    
                    llvm::Constant* constantValue = nullptr;
                    if (fieldType == VarType::FLOAT32) {
                        constantValue = llvm::ConstantFP::get(llvm::Type::getFloatTy(llvmContext), (float)floatExpr->getValue());
                    } else if (fieldType == VarType::FLOAT64) {
                        constantValue = llvm::ConstantFP::get(llvm::Type::getDoubleTy(llvmContext), floatExpr->getValue());
                    }
                    
                    if (constantValue) {
                        context.registerStructFieldDefault(structName, fieldName, constantValue);
                        std::cout << "DEBUG: Registered float default value for field '" << fieldName << "': " << floatExpr->getValue() << std::endl;
                    }
                }
                else if (auto* numberExpr = dynamic_cast<AST::NumberExpr*>(defaultValueExpr.get())) {
                    VarType fieldType = VarType::VOID;
                    for (const auto& field : decl.getFields()) {
                        if (field.first == fieldName) {
                            fieldType = field.second;
                            break;
                        }
                    }
                    
                    llvm::Constant* constantValue = nullptr;
                    const BigInt& bigValue = numberExpr->getValue();
                    
                    if (fieldType == VarType::INT8) {
                        constantValue = llvm::ConstantInt::get(llvm::Type::getInt8Ty(llvmContext), bigValue.toInt64(), true);
                    } else if (fieldType == VarType::INT16) {
                        constantValue = llvm::ConstantInt::get(llvm::Type::getInt16Ty(llvmContext), bigValue.toInt64(), true);
                    } else if (fieldType == VarType::INT32) {
                        constantValue = llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvmContext), bigValue.toInt64(), true);
                    } else if (fieldType == VarType::INT64) {
                        constantValue = llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvmContext), bigValue.toInt64(), true);
                    } else if (fieldType == VarType::UINT8) {
                        constantValue = llvm::ConstantInt::get(llvm::Type::getInt8Ty(llvmContext), bigValue.toInt64(), false);
                    } else if (fieldType == VarType::UINT16) {
                        constantValue = llvm::ConstantInt::get(llvm::Type::getInt16Ty(llvmContext), bigValue.toInt64(), false);
                    } else if (fieldType == VarType::UINT32) {
                        constantValue = llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvmContext), bigValue.toInt64(), false);
                    } else if (fieldType == VarType::UINT64) {
                        constantValue = llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvmContext), bigValue.toInt64(), false);
                    }
                    
                    if (constantValue) {
                        context.registerStructFieldDefault(structName, fieldName, constantValue);
                        std::cout << "DEBUG: Registered integer default value for field '" << fieldName << "': " << bigValue.toString() << std::endl;
                    }
                }
                else if (auto* boolExpr = dynamic_cast<AST::BooleanExpr*>(defaultValueExpr.get())) {
                    llvm::Constant* constantValue = llvm::ConstantInt::get(llvm::Type::getInt1Ty(llvmContext), boolExpr->getValue() ? 1 : 0);
                    context.registerStructFieldDefault(structName, fieldName, constantValue);
                    std::cout << "DEBUG: Registered boolean default value for field '" << fieldName << "': " << boolExpr->getValue() << std::endl;
                }
                else {
                    std::cout << "WARNING: Unsupported default value type for field '" << fieldName << "'" << std::endl;
                }
                
            } catch (const std::exception& e) {
                std::cout << "WARNING: Could not evaluate default value for field '" << fieldName << "': " << e.what() << std::endl;
            }
        }
    }
    
    
    std::cout << "DEBUG: Registered struct type '" << structName << "' with " << fieldTypes.size() << " fields" << std::endl;

    for (const auto& method : decl.getMethods()) {
        std::cout << "DEBUG: Generating method declaration for '" << method->getName() 
                  << "' for struct '" << structName << "'" << std::endl;
        std::cout << "DEBUG: Method return type: " << static_cast<int>(method->getReturnType())
                  << ", return struct name: '" << method->getReturnStructName() << "'" << std::endl;

        std::string mangledName = method->getName();
        std::cout << "DEBUG: Using mangled name: '" << mangledName << "'" << std::endl;
        
        std::vector<llvm::Type*> paramTypes;
        
        paramTypes.push_back(llvm::PointerType::get(structType, 0));
        std::cout << "DEBUG: Added self parameter of type: ";
        structType->print(llvm::errs());
        std::cout << " for method " << mangledName << std::endl;
        
        const auto& params = method->getParameters();
        for (size_t i = 0; i < params.size(); ++i) {
            const auto& param = params[i];
            
            if (param.first == "self") {
                std::cout << "DEBUG: Skipping 'self' parameter from method definition" << std::endl;
                continue;
            }
            
            llvm::Type* paramType = nullptr;
            if (param.second == VarType::STRUCT) {
                std::string paramStructName = method->getParameterStructName(i);

                if (paramStructName.empty()) {
                    paramStructName = structName;
                    std::cout << "DEBUG: Inferred struct name '" << paramStructName << "' for parameter '" << param.first << "'" << std::endl;
                }
                
                if (paramStructName.empty()) {
                    throw std::runtime_error("Struct type requires a struct name for parameter: " + param.first);
                }
                paramType = llvm::PointerType::get(context.getStructType(paramStructName), 0);
            } else {
                paramType = context.getLLVMType(param.second);
            }
            
            if (!paramType) {
                throw std::runtime_error("Unknown parameter type for method parameter: " + param.first);
            }
            paramTypes.push_back(paramType);
            std::cout << "DEBUG: Added parameter '" << param.first << "' type: ";
            paramType->print(llvm::errs());
            std::cout << std::endl;
        }
        
        llvm::Type* returnType = nullptr;
        if (method->getReturnType() == VarType::STRUCT) {
            std::string returnStructName = method->getReturnStructName();
            std::cout << "DEBUG: Method returns struct: '" << returnStructName << "'" << std::endl;
            
            if (returnStructName.empty()) {
                throw std::runtime_error("Struct type requires a struct name for method: " + method->getName());
            }
            
            returnType = context.getStructType(returnStructName);
            if (!returnType) {
                throw std::runtime_error("Unknown struct return type: " + returnStructName);
            }
        } else {
            returnType = context.getLLVMType(method->getReturnType());
        }
        
        if (!returnType) {
            throw std::runtime_error("Unknown return type for method: " + method->getName());
        }
        
        auto funcType = llvm::FunctionType::get(returnType, paramTypes, false);
        
        auto function = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            mangledName,
            &module
        );

        unsigned argIdx = 0;
        for (auto& arg : function->args()) {
            if (argIdx == 0) {
                arg.setName("self");
                std::cout << "DEBUG: Set parameter " << argIdx << " name to 'self'" << std::endl;
            } else {
                size_t methodParamIdx = 0;
                size_t nonSelfParamCount = 0;
                
                for (size_t i = 0; i < params.size(); ++i) {
                    if (params[i].first != "self") {
                        if (nonSelfParamCount == argIdx - 1) {
                            methodParamIdx = i;
                            break;
                        }
                        nonSelfParamCount++;
                    }
                }
                
                if (methodParamIdx < params.size()) {
                    std::string paramName = params[methodParamIdx].first;
                    arg.setName(paramName);
                    std::cout << "DEBUG: Set parameter " << argIdx << " name to '" << paramName << "'" << std::endl;
                }
            }
            argIdx++;
        }
        
        std::cout << "DEBUG: Created method declaration for '" << mangledName << "' with " 
                  << function->arg_size() << " parameters:" << std::endl;
        argIdx = 0;
        for (auto& arg : function->args()) {
            std::cout << "  Param " << argIdx << ": " << arg.getName().str() << " - ";
            arg.getType()->print(llvm::errs());
            std::cout << std::endl;
            argIdx++;
        }
    }
    
    std::cout << "DEBUG: Successfully generated struct '" << structName << "' type definition" << std::endl;
    return nullptr;
}

void StatementCodeGen::codegenStructMethodBodies(CodeGen& context, StructDecl& decl) {
    auto& llvmContext = context.getContext();
    auto& module = context.getModule();
    
    std::string structName = decl.getName();
    llvm::StructType* structType = llvm::cast<llvm::StructType>(context.getStructType(structName));
    
    std::cout << "DEBUG codegenStructMethodBodies: Generating method bodies for struct '" << structName << "'" << std::endl;
    
    for (const auto& method : decl.getMethods()) {
        std::string mangledName = method->getName();
        std::cout << "DEBUG: Looking for method declaration: '" << mangledName << "'" << std::endl;
        
        auto function = module.getFunction(mangledName);
        
        if (!function) {
            std::string altName = structName + "." + method->getName().substr(structName.length() + 1);
            std::cout << "DEBUG: Trying alternative name: '" << altName << "'" << std::endl;
            function = module.getFunction(altName);
        }
        
        if (!function) {
            throw std::runtime_error("Method declaration not found: " + mangledName);
        }
        
        if (method->getBody()) {
            std::cout << "DEBUG: Generating method body for '" << mangledName << "'" << std::endl;
            
            auto& builder = context.getBuilder();
            llvm::BasicBlock* savedInsertBlock = builder.GetInsertBlock();
            
            auto savedNamedValues = context.getNamedValues();
            auto savedVariableTypes = context.getVariableTypes();
            
            context.enterScope();
            
            std::cout << "DEBUG: Copying global variables into method scope for '" << mangledName << "':" << std::endl;
            for (const auto& [varName, varValue] : savedNamedValues) {
                if (context.isGlobalVariable(varName)) {
                    context.getNamedValues()[varName] = varValue;
                    auto it = savedVariableTypes.find(varName);
                    if (it != savedVariableTypes.end()) {
                        context.getVariableTypes()[varName] = it->second;
                    }
                    std::cout << "  - Copied global: " << varName << std::endl;
                }
            }
            
            auto entryBlock = llvm::BasicBlock::Create(llvmContext, "entry", function);
            builder.SetInsertPoint(entryBlock);
            
            std::cout << "DEBUG: In method '" << mangledName << "', available variables in scope:" << std::endl;
            auto& currentNamedValues = context.getNamedValues();
            for (const auto& var : currentNamedValues) {
                std::cout << "  - " << var.first;
                if (context.isGlobalVariable(var.first)) {
                    std::cout << " (global)";
                }
                std::cout << std::endl;
            }

            unsigned idx = 0;
            const auto& methodParams = method->getParameters();
            for (auto& arg : function->args()) {
                if (idx == 0 && arg.getName() == "self") {
                    arg.setName("self");
                    context.getNamedValues()["self"] = &arg;
                    context.getVariableTypes()["self"] = VarType::STRUCT;
                    context.setVariableStructName("self", structName);
                    std::cout << "DEBUG: Set variable 'self' to struct '" << structName << "' (direct argument)" << std::endl;
                } else {
                    size_t methodParamIdx = 0;
                    size_t nonSelfParamCount = 0;
                    
                    for (size_t i = 0; i < methodParams.size(); ++i) {
                        if (methodParams[i].first != "self") {
                            if (nonSelfParamCount == idx - 1) {
                                methodParamIdx = i;
                                break;
                            }
                            nonSelfParamCount++;
                        }
                    }
                    
                    if (methodParamIdx < methodParams.size()) {
                        std::string paramName = methodParams[methodParamIdx].first;
                        VarType paramType = methodParams[methodParamIdx].second;
                        
                        arg.setName(paramName);

                        if (paramType == VarType::STRUCT) {
                            context.getNamedValues()[paramName] = &arg;
                            context.getVariableTypes()[paramName] = paramType;
                            context.setVariableStructName(paramName, structName);
                            std::cout << "DEBUG: Set struct parameter '" << paramName << "' to struct '" 
                                      << structName << "' (direct argument)" << std::endl;
                        } else {
                            auto alloca = builder.CreateAlloca(arg.getType(), nullptr, paramName);
                            builder.CreateStore(&arg, alloca);
                            context.getNamedValues()[paramName] = alloca;
                            context.getVariableTypes()[paramName] = paramType;
                            
                            std::cout << "DEBUG: Set parameter '" << paramName << "' with type " 
                                      << static_cast<int>(paramType) << std::endl;
                        }
                    }
                }
                idx++;
            }
            
            method->getBody()->codegen(context);
            
            auto currentBlock = builder.GetInsertBlock();
            if (!currentBlock->getTerminator()) {
                if (method->getReturnType() == VarType::VOID) {
                    builder.CreateRetVoid();
                } else {
                    throw std::runtime_error("Method must return a value");
                }
            }
            
            if (llvm::verifyFunction(*function, &llvm::errs())) {
                function->print(llvm::errs());
                throw std::runtime_error("Method failed verification");
            }
            
            context.exitScope();
            
            if (savedInsertBlock) {
                builder.SetInsertPoint(savedInsertBlock);
            }
        }
    }
}

llvm::Value* StatementCodeGen::codegenMemberAssignment(CodeGen& context, MemberAssignmentStmt& stmt) {
    auto& builder = context.getBuilder();
    
    auto* varExpr = dynamic_cast<AST::VariableExpr*>(stmt.getObject().get());
    if (!varExpr) {
        throw std::runtime_error("Member assignment only supports direct variable access");
    }
    
    std::string varName = varExpr->getName();
    auto var = context.lookupVariable(varName);
    if (!var) {
        throw std::runtime_error("Unknown variable: " + varName);
    }
    
    auto varType = context.lookupVariableType(varName);
    if (varType != VarType::STRUCT) {
        throw std::runtime_error("Cannot access member of non-struct variable: " + varName);
    }
    
    std::string structName;
    llvm::StructType* structType = nullptr;
    
    if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(var)) {
        llvm::Type* allocatedType = alloca->getAllocatedType();
        if (allocatedType->isStructTy()) {
            structType = llvm::cast<llvm::StructType>(allocatedType);
            structName = structType->getName().str();
        }
    } else if (auto* globalVar = llvm::dyn_cast<llvm::GlobalVariable>(var)) {
        llvm::Type* valueType = globalVar->getValueType();
        if (valueType->isStructTy()) {
            structType = llvm::cast<llvm::StructType>(valueType);
            structName = structType->getName().str();
        }
    }
    
    if (!structType || structName.empty()) {
        throw std::runtime_error("Could not determine struct type for variable: " + varName);
    }
    
    int fieldIndex = context.getStructFieldIndex(structName, stmt.getMemberName());
    if (fieldIndex == -1) {
        throw std::runtime_error("Unknown field '" + stmt.getMemberName() + "' in struct '" + structName + "'");
    }
    
    auto value = stmt.getValue()->codegen(context);
    
    llvm::Value* fieldPtr = builder.CreateStructGEP(structType, var, fieldIndex, stmt.getMemberName());
    
    llvm::Type* expectedFieldType = structType->getElementType(fieldIndex);
    
    if (value->getType() != expectedFieldType) {
        if (expectedFieldType->isIntegerTy() && value->getType()->isIntegerTy()) {
            unsigned expectedBits = expectedFieldType->getIntegerBitWidth();
            unsigned actualBits = value->getType()->getIntegerBitWidth();
            
            if (actualBits > expectedBits) {
                value = builder.CreateTrunc(value, expectedFieldType);
            } else if (actualBits < expectedBits) {
                VarType sourceType = AST::inferSourceType(value, context);
                if (AST::TypeBounds::isUnsignedType(sourceType)) {
                    value = builder.CreateZExt(value, expectedFieldType);
                } else {
                    value = builder.CreateSExt(value, expectedFieldType);
                }
            }
        } else if (expectedFieldType->isFPOrFPVectorTy() && value->getType()->isIntegerTy()) {
            value = builder.CreateSIToFP(value, expectedFieldType);
        } else if (expectedFieldType->isIntegerTy() && value->getType()->isFPOrFPVectorTy()) {
            value = builder.CreateFPToSI(value, expectedFieldType);
        }
    }
    
    builder.CreateStore(value, fieldPtr);
    
    return value;
}