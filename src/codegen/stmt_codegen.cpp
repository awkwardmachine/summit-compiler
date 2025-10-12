#include "stmt_codegen.h"
#include "codegen.h"
#include "ast/ast.h"
#include <llvm/IR/Verifier.h>
#include <llvm/IR/BasicBlock.h>
#include "codegen/bounds.h"
#include "type_inference.h"
#include "expr_codegen.h"

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
    
    if (isGlobal) {
        llvm::Type* llvmType = context.getLLVMType(type);
        if (!llvmType) {
            throw std::runtime_error("Unknown type for global variable: " + name);
        }
        
        if (valueExpr) {
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
        llvm::Type* llvmType = context.getLLVMType(type);
        if (!llvmType) {
            throw std::runtime_error("Unknown type for variable: " + name);
        }
        
        llvm::Function* currentFunction = builder.GetInsertBlock()->getParent();
        llvm::IRBuilder<> entryBuilder(&currentFunction->getEntryBlock(), currentFunction->getEntryBlock().begin());
        llvm::AllocaInst* alloca = entryBuilder.CreateAlloca(llvmType, nullptr, name);
        
        if (valueExpr) {
            auto value = valueExpr->codegen(context);
            
            if (value->getType() != llvmType) {
                if (llvmType->isIntegerTy() && value->getType()->isIntegerTy()) {
                    unsigned targetBits = llvmType->getIntegerBitWidth();
                    unsigned sourceBits = value->getType()->getIntegerBitWidth();
                    
                    if (sourceBits < targetBits) {
                        if (TypeBounds::isUnsignedType(type)) {
                            value = builder.CreateZExt(value, llvmType);
                        } else {
                            value = builder.CreateSExt(value, llvmType);
                        }
                    } else if (sourceBits > targetBits) {
                        value = builder.CreateTrunc(value, llvmType);
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
    
    if (value->getType() != expectedType) {
        if (expectedType->isFPOrFPVectorTy() && value->getType()->isFPOrFPVectorTy()) {
            if (varType == VarType::FLOAT32 && value->getType()->isDoubleTy()) {
                value = builder.CreateFPTrunc(value, expectedType);
            } else if (varType == VarType::FLOAT64 && value->getType()->isFloatTy()) {
                value = builder.CreateFPExt(value, expectedType);
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
    
    auto varType = context.getLLVMType(decl.getType());
    if (!varType) {
        throw std::runtime_error("Unknown variable type for global: " + decl.getName());
    }
    
    llvm::Constant* initialValue = nullptr;
    
    if (decl.getValue()) {
        if (auto* moduleExpr = dynamic_cast<ModuleExpr*>(decl.getValue().get())) {
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
                context.getVariableTypes()[decl.getName()] = decl.getType();
                
                auto stdModule = context.lookupVariable("std");
                if (stdModule) {
                    context.setModuleReference(decl.getName(), stdModule, "std");
                    std::cout << "DEBUG: Created global module alias: " << decl.getName() << " -> std" << std::endl;
                }
                
                if (decl.getIsConst()) {
                    context.getConstVariables().insert(decl.getName());
                }
                
                return globalVar;
            } else {
                throw std::runtime_error("Unknown module: " + moduleName);
            }
        }
        else if (auto* memberAccess = dynamic_cast<MemberAccessExpr*>(decl.getValue().get())) {
            if (auto* varExpr = dynamic_cast<VariableExpr*>(memberAccess->getObject().get())) {
                std::string moduleName = varExpr->getName();
                std::string memberName = memberAccess->getMember();
                
                auto baseVar = context.lookupVariable(moduleName);
                if (baseVar) {
                    auto baseType = context.lookupVariableType(moduleName);
                    if (baseType == VarType::MODULE) {
                        std::string actualModuleName = context.getModuleIdentity(moduleName);
                        if (!actualModuleName.empty()) {
                            moduleName = actualModuleName;
                        }
                        
                        if (moduleName == "std" && memberName == "io") {
                            auto ioType = context.getLLVMType(VarType::MODULE);
                            initialValue = ConstantAggregateZero::get(ioType);
                            
                            auto globalVar = new llvm::GlobalVariable(
                                module,
                                ioType,
                                decl.getIsConst(),
                                llvm::GlobalValue::ExternalLinkage,
                                initialValue,
                                decl.getName()
                            );
                            
                            context.getNamedValues()[decl.getName()] = globalVar;
                            context.getVariableTypes()[decl.getName()] = VarType::MODULE;
                            context.setModuleReference(decl.getName(), globalVar, "io");
                            
                            std::cout << "DEBUG: Created global module alias: " << decl.getName() << " -> io" << std::endl;
                            
                            if (decl.getIsConst()) {
                                context.getConstVariables().insert(decl.getName());
                            }
                            
                            return globalVar;
                        }
                    }
                }
            }
            throw std::runtime_error("Global variables can only be initialized with constant expressions or module members");
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
            throw std::runtime_error("Global variables can only be initialized with constant expressions");
        }
    } else {
        if (decl.getType() == VarType::FLOAT32) {
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
    
    return globalVar;
}

llvm::Value* StatementCodeGen::codegenProgram(CodeGen& context, Program& program) {
    auto& builder = context.getBuilder();
    auto& module = context.getModule();
    auto& llvmContext = context.getContext();
    
    std::cout << "DEBUG: First pass - generating enum declarations" << std::endl;
    
    for (size_t i = 0; i < program.getStatements().size(); i++) {
        auto& stmt = program.getStatements()[i];
        if (auto* enumDecl = dynamic_cast<EnumDecl*>(stmt.get())) {
            std::cout << "DEBUG: Generating enum: " << enumDecl->getName() << std::endl;
            enumDecl->codegen(context);
        }
    }
    
    std::cout << "DEBUG: Second pass - generating global variables and functions" << std::endl;

    for (size_t i = 0; i < program.getStatements().size(); i++) {
        auto& stmt = program.getStatements()[i];
        if (auto* varDecl = dynamic_cast<VariableDecl*>(stmt.get())) {
            std::cout << "DEBUG: Generating global variable: " << varDecl->getName() << std::endl;
            codegenGlobalVariable(context, *varDecl);
        }
    }

    for (size_t i = 0; i < program.getStatements().size(); i++) {
        auto& stmt = program.getStatements()[i];
        if (auto* funcStmt = dynamic_cast<FunctionStmt*>(stmt.get())) {
            std::cout << "DEBUG: Generating function: " << funcStmt->getName() << std::endl;
            funcStmt->codegen(context);
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
    for (const auto& param : stmt.getParameters()) {
        auto paramType = context.getLLVMType(param.second);
        if (!paramType) {
            throw std::runtime_error("Unknown parameter type for: " + param.first);
        }
        paramTypes.push_back(paramType);
    }
    
    auto returnType = context.getLLVMType(stmt.getReturnType());
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
            
            auto alloca = builder.CreateAlloca(arg.getType(), nullptr, param.first);
            builder.CreateStore(&arg, alloca);
            
            context.getNamedValues()[param.first] = alloca;
            context.getVariableTypes()[param.first] = param.second;
            idx++;
        }
        
        stmt.getBody()->codegen(context);
        
        auto currentBlock = builder.GetInsertBlock();
        if (!currentBlock->getTerminator()) {
            if (stmt.getReturnType() == VarType::VOID) {
                builder.CreateRetVoid();
            } else {
                throw std::runtime_error("Function '" + stmt.getName() + 
                                       "' with return type '" + 
                                       TypeBounds::getTypeName(stmt.getReturnType()) + 
                                       "' must return a value on all code paths");
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
        if (expectedReturnType->isIntegerTy(1)) {
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

    context.enterScope();

    auto varType = stmt.getVarType();
    auto llvmVarType = context.getLLVMType(varType);
    IRBuilder<> entryBuilder(&currentFunction->getEntryBlock(),
                           currentFunction->getEntryBlock().begin());
    auto alloca = entryBuilder.CreateAlloca(llvmVarType, nullptr, stmt.getVarName());
   
    if (stmt.getInitializer()) {
        auto initValue = stmt.getInitializer()->codegen(context);

        if (auto numberExpr = dynamic_cast<NumberExpr*>(stmt.getInitializer().get())) {
            const BigInt& bigValue = numberExpr->getValue();
            if (!TypeBounds::checkBounds(varType, bigValue)) {
                throw std::runtime_error(
                    "Value " + bigValue.toString() + " out of bounds for type " + 
                    TypeBounds::getTypeName(varType) + ". " +
                    "Valid range: " + TypeBounds::getTypeRange(varType)
                );
            }
        }
        
        builder.CreateStore(initValue, alloca);
    } else {
        auto zero = ConstantInt::get(llvmVarType, 0);
        builder.CreateStore(zero, alloca);
    }

    context.getNamedValues()[stmt.getVarName()] = alloca;
    context.getVariableTypes()[stmt.getVarName()] = stmt.getVarType();

    if (auto binaryCond = dynamic_cast<BinaryExpr*>(stmt.getCondition().get())) {
        if (binaryCond->getOp() == BinaryOp::LESS) {
            if (auto limitNum = dynamic_cast<NumberExpr*>(binaryCond->getRHS().get())) {
                const BigInt& limit = limitNum->getValue();
                
                std::string typeRange = TypeBounds::getTypeRange(varType);
                size_t toPos = typeRange.find(" to ");
                if (toPos != std::string::npos) {
                    try {
                        int64_t maxVal = std::stoll(typeRange.substr(toPos + 4));
                        BigInt maxBound(maxVal);
                        
                        if (limit > maxBound) {
                            throw std::runtime_error(
                                "For loop condition " + stmt.getVarName() + " < " + limit.toString() + 
                                " would exceed bounds of type " + TypeBounds::getTypeName(varType) + 
                                ". Valid range: " + typeRange
                            );
                        }
                    } catch (...) {
                        
                    }
                }
            }
        }
        else if (binaryCond->getOp() == BinaryOp::LESS_EQUAL) {
            if (auto limitNum = dynamic_cast<NumberExpr*>(binaryCond->getRHS().get())) {
                const BigInt& limit = limitNum->getValue();
                
                std::string typeRange = TypeBounds::getTypeRange(varType);
                size_t toPos = typeRange.find(" to ");
                if (toPos != std::string::npos) {
                    try {
                        int64_t maxVal = std::stoll(typeRange.substr(toPos + 4));
                        BigInt maxBound(maxVal);
                        
                        if (limit > maxBound) {
                            throw std::runtime_error(
                                "For loop condition " + stmt.getVarName() + " <= " + limit.toString() + 
                                " would exceed bounds of type " + TypeBounds::getTypeName(varType) + 
                                ". Valid range: " + typeRange
                            );
                        }
                    } catch (...) {
                        
                    }
                }
            }
        }
    }
   
    builder.CreateBr(conditionBlock);
   
    builder.SetInsertPoint(conditionBlock);
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
   
    for (auto& bodyStmt : stmt.getBody()->getStatements()) {
        bodyStmt->codegen(context);
    }

    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(incrementBlock);
    }
   
    currentFunction->insert(currentFunction->end(), incrementBlock);
    builder.SetInsertPoint(incrementBlock);
   
    if (stmt.getIncrement()) {
        auto incrementValue = stmt.getIncrement()->codegen(context);
        if (incrementValue) {
            if (auto numberExpr = dynamic_cast<NumberExpr*>(stmt.getIncrement().get())) {
                const BigInt& bigValue = numberExpr->getValue();
                if (!TypeBounds::checkBounds(varType, bigValue)) {
                    throw std::runtime_error(
                        "Increment value " + bigValue.toString() + " out of bounds for type " + 
                        TypeBounds::getTypeName(varType) + ". " +
                        "Valid range: " + TypeBounds::getTypeRange(varType)
                    );
                }
            }
            
            builder.CreateStore(incrementValue, alloca);
        }
    }
   
    builder.CreateBr(conditionBlock);
   
    currentFunction->insert(currentFunction->end(), afterBlock);
    builder.SetInsertPoint(afterBlock);
   
    context.exitScope();
   
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