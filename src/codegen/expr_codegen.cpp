#include "expr_codegen.h"
#include "type_inference.h"
#include "string_conversions.h"
#include "format_utils.h"
#include "codegen/bounds.h"
#include "stdlib/core/stdlib_manager.h"

#include <llvm/IR/Verifier.h>
#include <llvm/IR/Type.h>
#include <iostream>
#include <regex>
#include <vector>
#include <sstream>
#include <cstdint>
#include <climits>
#include <limits>

using namespace llvm;
using namespace AST;

static llvm::Constant* createDefaultValue(llvm::Type* type, AST::VarType varType) {
    if (type->isIntegerTy()) {
        if (AST::TypeBounds::isUnsignedType(varType)) {
            return llvm::ConstantInt::get(type, 0, false);
        } else {
            return llvm::ConstantInt::get(type, 0, true);
        }
    } else if (type->isFloatTy()) {
        return llvm::ConstantFP::get(type, 0.0f);
    } else if (type->isDoubleTy()) {
        return llvm::ConstantFP::get(type, 0.0);
    } else if (type->isPointerTy()) {
        return llvm::ConstantPointerNull::get(static_cast<llvm::PointerType*>(type));
    }
    return nullptr;
}

llvm::Value* ExpressionCodeGen::codegenString(CodeGen& context, StringExpr& expr) {
    auto& builder = context.getBuilder();
    const std::string& strValue = expr.getValue();
    auto* strConstant = builder.CreateGlobalStringPtr(strValue);
    return strConstant;
}

llvm::Value* ExpressionCodeGen::codegenNumber(CodeGen& context, NumberExpr& expr) {
    const BigInt& value = expr.getValue();
    std::string strValue = value.toString();
   
    bool isNegative = false;
    size_t startPos = 0;
   
    if (!strValue.empty() && strValue[0] == '-') {
        isNegative = true;
        startPos = 1;
    }
   
    uint64_t absValue = 0;
    
    if (strValue.length() >= 2 && strValue.substr(startPos, 2) == "0b") {
        std::string binStr = strValue.substr(startPos + 2);
        
        binStr.erase(std::remove(binStr.begin(), binStr.end(), '_'), binStr.end());
        
        if (binStr.empty()) {
            throw std::runtime_error("Invalid binary literal: " + strValue);
        }
        
        for (char c : binStr) {
            if (c != '0' && c != '1') {
                throw std::runtime_error("Invalid binary digit: " + std::string(1, c));
            }
            absValue = (absValue << 1) | (c - '0');
        }
        
        if (isNegative) {
            return ConstantInt::get(context.getContext(), APInt(64, -static_cast<int64_t>(absValue), true));
        } else {
            return ConstantInt::get(context.getContext(), APInt(64, absValue, false));
        }
    }
    
    if (strValue.length() >= 2 && strValue.substr(startPos, 2) == "0x") {
        std::string hexStr = strValue.substr(startPos + 2);
        
        hexStr.erase(std::remove(hexStr.begin(), hexStr.end(), '_'), hexStr.end());
        
        if (hexStr.empty()) {
            throw std::runtime_error("Invalid hex literal: " + strValue);
        }
        
        for (char c : hexStr) {
            int digit;
            if (c >= '0' && c <= '9') digit = c - '0';
            else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
            else throw std::runtime_error("Invalid hex digit: " + std::string(1, c));
            
            absValue = (absValue << 4) | digit;
        }
        
        if (isNegative) {
            return ConstantInt::get(context.getContext(), APInt(64, -static_cast<int64_t>(absValue), true));
        } else {
            return ConstantInt::get(context.getContext(), APInt(64, absValue, false));
        }
    }
    
    for (size_t i = startPos; i < strValue.length(); i++) {
        if (strValue[i] >= '0' && strValue[i] <= '9') {
            absValue = absValue * 10 + (strValue[i] - '0');
        }
    }

    if (!isNegative && absValue > INT64_MAX) {
        return ConstantInt::get(context.getContext(), APInt(64, absValue, false));
    } else {
        int64_t int64Value = isNegative ? -static_cast<int64_t>(absValue) : static_cast<int64_t>(absValue);
        return ConstantInt::get(context.getContext(), APInt(64, int64Value, true));
    }
}

llvm::Value* ExpressionCodeGen::codegenFloat(CodeGen& context, FloatExpr& expr) {
    auto& builder = context.getBuilder();
    switch (expr.getFloatType()) {
        case AST::VarType::FLOAT32:
            return ConstantFP::get(Type::getFloatTy(context.getContext()), (float)expr.getValue());
        case AST::VarType::FLOAT64:
            return ConstantFP::get(Type::getDoubleTy(context.getContext()), expr.getValue());
        default:
            throw std::runtime_error("Unsupported float type");
    }
}
llvm::Value* ExpressionCodeGen::codegenVariable(CodeGen& context, VariableExpr& expr) {
    std::string name = expr.getName();
    
    std::cout << "DEBUG codegenVariable: Looking for variable '" << name << "'" << std::endl;
    
    auto var = context.lookupVariable(name);
    if (!var) {
        throw std::runtime_error("Unknown variable: " + name);
    }
    
    std::cout << "DEBUG codegenVariable: Found variable '" << name << "' with value " << var << std::endl;
    
    VarType varType = context.lookupVariableType(name);
    std::cout << "DEBUG codegenVariable: Variable type = " << static_cast<int>(varType) << std::endl;

    if (varType == VarType::MODULE) {
        std::cout << "DEBUG codegenVariable: Returning module reference without loading for: " << name << std::endl;
        return var;
    }
    
    auto& builder = context.getBuilder();
    
    std::cout << "DEBUG codegenVariable: Loading value from pointer for: " << name << std::endl;
    
    llvm::Type* pointedType = nullptr;
    if (auto* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(var)) {
        pointedType = allocaInst->getAllocatedType();
    } else if (auto* globalVar = llvm::dyn_cast<llvm::GlobalVariable>(var)) {
        pointedType = globalVar->getValueType();
    }
    
    if (pointedType) {
        std::string typeStr;
        llvm::raw_string_ostream rso(typeStr);
        pointedType->print(rso);
        std::cout << "DEBUG codegenVariable: Pointed type = " << rso.str() << std::endl;
    }
    
    if (!pointedType) {
        throw std::runtime_error("Unable to determine type for variable: " + name);
    }
    
    return builder.CreateLoad(pointedType, var, name);
}

llvm::Value* ExpressionCodeGen::codegenUnary(CodeGen& context, UnaryExpr& expr) {
    auto& builder = context.getBuilder();
    auto operand = expr.getOperand()->codegen(context);
    
    switch (expr.getOp()) {
        case UnaryOp::LOGICAL_NOT: {
            if (!operand->getType()->isIntegerTy(1)) {
                if (operand->getType()->isIntegerTy()) {
                    operand = builder.CreateICmpNE(operand, 
                        ConstantInt::get(operand->getType(), 0));
                } else if (operand->getType()->isFPOrFPVectorTy()) {
                    operand = builder.CreateFCmpONE(operand,
                        ConstantFP::get(operand->getType(), 0.0));
                } else {
                    throw std::runtime_error("Cannot apply NOT to this type");
                }
            }
            return builder.CreateNot(operand, "nottmp");
        }
        
        case UnaryOp::NEGATE: {
            if (operand->getType()->isFPOrFPVectorTy()) {
                return builder.CreateFNeg(operand, "negtmp");
            } else if (operand->getType()->isIntegerTy()) {
                return builder.CreateNeg(operand, "negtmp");
            } else {
                throw std::runtime_error("Cannot negate this type");
            }
        }
        
        case UnaryOp::BITWISE_NOT: {
            if (operand->getType()->isIntegerTy()) {
                if (operand->getType()->getIntegerBitWidth() == 1) {
                    operand = builder.CreateZExt(operand, Type::getInt64Ty(context.getContext()));
                }
                return builder.CreateNot(operand, "bwnottmp");
            } else if (operand->getType()->isFPOrFPVectorTy()) {
                auto intVal = builder.CreateFPToSI(operand, Type::getInt64Ty(context.getContext()));
                auto notVal = builder.CreateNot(intVal, "bwnottmp");
                return builder.CreateSIToFP(notVal, operand->getType());
            } else {
                throw std::runtime_error("Cannot apply bitwise NOT to this type");
            }
        }
        
        default:
            throw std::runtime_error("Unknown unary operator");
    }
}
llvm::Value* ExpressionCodeGen::codegenBinary(CodeGen& context, BinaryExpr& expr) {
    auto lhs = expr.getLHS()->codegen(context);
    auto rhs = expr.getRHS()->codegen(context);
    auto& builder = context.getBuilder();

    if (expr.getOp() == BinaryOp::ADD) {
        bool lhsIsString = lhs->getType()->isPointerTy();
        bool rhsIsString = rhs->getType()->isPointerTy();
        
        if (lhsIsString || rhsIsString) {
            if (!lhsIsString) {
                throw std::runtime_error(
                    "Left operand must be a string for concatenation. " +
                    std::string("Use 'as str' to explicitly convert: ") +
                    "(left_expression as str) + right_expression"
                );
            }
            if (!rhsIsString) {
                throw std::runtime_error(
                    "Right operand must be a string for concatenation. " +
                    std::string("Use 'as str' to explicitly convert: ") +
                    "left_expression + (right_expression as str)"
                );
            }
            
            auto& module = context.getModule();
            auto& llvmContext = context.getContext();
            
            auto strlenFunc = module.getFunction("strlen");
            if (!strlenFunc) {
                auto* i8Ptr = PointerType::get(Type::getInt8Ty(llvmContext), 0);
                auto* sizeT = Type::getInt64Ty(llvmContext);
                auto strlenType = FunctionType::get(sizeT, {i8Ptr}, false);
                strlenFunc = Function::Create(strlenType, Function::ExternalLinkage, "strlen", &module);
            }
            
            auto mallocFunc = module.getFunction("malloc");
            if (!mallocFunc) {
                auto* i8Ptr = PointerType::get(Type::getInt8Ty(llvmContext), 0);
                auto* sizeT = Type::getInt64Ty(llvmContext);
                auto mallocType = FunctionType::get(i8Ptr, {sizeT}, false);
                mallocFunc = Function::Create(mallocType, Function::ExternalLinkage, "malloc", &module);
            }
            
            auto strcpyFunc = module.getFunction("strcpy");
            if (!strcpyFunc) {
                auto* i8Ptr = PointerType::get(Type::getInt8Ty(llvmContext), 0);
                std::vector<llvm::Type*> paramTypes = {i8Ptr, i8Ptr};
                auto funcType = FunctionType::get(i8Ptr, paramTypes, false);
                strcpyFunc = Function::Create(funcType, Function::ExternalLinkage, "strcpy", &module);
            }
            
            auto strcatFunc = module.getFunction("strcat");
            if (!strcatFunc) {
                auto* i8Ptr = PointerType::get(Type::getInt8Ty(llvmContext), 0);
                std::vector<llvm::Type*> paramTypes = {i8Ptr, i8Ptr};
                auto funcType = FunctionType::get(i8Ptr, paramTypes, false);
                strcatFunc = Function::Create(funcType, Function::ExternalLinkage, "strcat", &module);
            }
            
            auto lhsLen = builder.CreateCall(strlenFunc, {lhs});
            auto rhsLen = builder.CreateCall(strlenFunc, {rhs});
            auto totalLen = builder.CreateAdd(lhsLen, rhsLen);
            auto bufferSize = builder.CreateAdd(totalLen, ConstantInt::get(llvmContext, APInt(64, 1)));
            
            auto buffer = builder.CreateCall(mallocFunc, {bufferSize});
            builder.CreateCall(strcpyFunc, {buffer, lhs});
            builder.CreateCall(strcatFunc, {buffer, rhs});
            
            return buffer;
        }
    }

    if (expr.getOp() == BinaryOp::LOGICAL_AND || expr.getOp() == BinaryOp::LOGICAL_OR) {
        if (!lhs->getType()->isIntegerTy(1)) {
            if (lhs->getType()->isIntegerTy()) {
                lhs = builder.CreateICmpNE(lhs, ConstantInt::get(lhs->getType(), 0));
            } else if (lhs->getType()->isFPOrFPVectorTy()) {
                lhs = builder.CreateFCmpONE(lhs, ConstantFP::get(lhs->getType(), 0.0));
            }
        }

        if (!rhs->getType()->isIntegerTy(1)) {
            if (rhs->getType()->isIntegerTy()) {
                rhs = builder.CreateICmpNE(rhs, ConstantInt::get(rhs->getType(), 0));
            } else if (rhs->getType()->isFPOrFPVectorTy()) {
                rhs = builder.CreateFCmpONE(rhs, ConstantFP::get(rhs->getType(), 0.0));
            }
        }
        
        if (expr.getOp() == BinaryOp::LOGICAL_AND) {
            return builder.CreateAnd(lhs, rhs, "andtmp");
        } else {
            return builder.CreateOr(lhs, rhs, "ortmp");
        }
    }

    bool lhsIsFloat = lhs->getType()->isFPOrFPVectorTy();
    bool rhsIsFloat = rhs->getType()->isFPOrFPVectorTy();
    
    if (lhsIsFloat || rhsIsFloat) {
        llvm::Type* resultType = nullptr;
        if (lhs->getType()->isDoubleTy() || rhs->getType()->isDoubleTy()) {
            resultType = Type::getDoubleTy(context.getContext());
        } else {
            resultType = Type::getFloatTy(context.getContext());
        }

        if (lhs->getType()->isIntegerTy()) {
            lhs = builder.CreateSIToFP(lhs, resultType);
        } else if (lhs->getType() != resultType) {
            if (lhs->getType()->isFloatTy() && resultType->isDoubleTy()) {
                lhs = builder.CreateFPExt(lhs, resultType);
            }
        }
        
        if (rhs->getType()->isIntegerTy()) {
            rhs = builder.CreateSIToFP(rhs, resultType);
        } else if (rhs->getType() != resultType) {
            if (rhs->getType()->isFloatTy() && resultType->isDoubleTy()) {
                rhs = builder.CreateFPExt(rhs, resultType);
            }
        }

        switch (expr.getOp()) {
            case BinaryOp::ADD: return builder.CreateFAdd(lhs, rhs, "addtmp");
            case BinaryOp::SUBTRACT: return builder.CreateFSub(lhs, rhs, "subtmp");
            case BinaryOp::MULTIPLY: return builder.CreateFMul(lhs, rhs, "multmp");
            case BinaryOp::DIVIDE: return builder.CreateFDiv(lhs, rhs, "divtmp");
            case BinaryOp::MODULUS: 
                {
                    auto& module = context.getModule();
                    auto fmodFunc = module.getFunction("fmod");
                    if (!fmodFunc) {
                        auto* doubleTy = Type::getDoubleTy(context.getContext());
                        std::vector<llvm::Type*> paramTypes = {doubleTy, doubleTy};
                        auto funcType = FunctionType::get(doubleTy, paramTypes, false);
                        fmodFunc = Function::Create(funcType, Function::ExternalLinkage, "fmod", &module);
                    }
                    if (lhs->getType()->isFloatTy()) {
                        lhs = builder.CreateFPExt(lhs, Type::getDoubleTy(context.getContext()));
                    }
                    if (rhs->getType()->isFloatTy()) {
                        rhs = builder.CreateFPExt(rhs, Type::getDoubleTy(context.getContext()));
                    }
                    return builder.CreateCall(fmodFunc, {lhs, rhs}, "fmodtmp");
                }
            case BinaryOp::GREATER: return builder.CreateFCmpOGT(lhs, rhs, "gttmp");
            case BinaryOp::LESS: return builder.CreateFCmpOLT(lhs, rhs, "lttmp");
            case BinaryOp::GREATER_EQUAL: return builder.CreateFCmpOGE(lhs, rhs, "getmp");
            case BinaryOp::LESS_EQUAL: return builder.CreateFCmpOLE(lhs, rhs, "letmp");
            case BinaryOp::EQUAL: return builder.CreateFCmpOEQ(lhs, rhs, "eqtmp");
            case BinaryOp::NOT_EQUAL: return builder.CreateFCmpONE(lhs, rhs, "netmp");
            default: 
                if (expr.getOp() == BinaryOp::BITWISE_AND || 
                    expr.getOp() == BinaryOp::BITWISE_OR || 
                    expr.getOp() == BinaryOp::BITWISE_XOR ||
                    expr.getOp() == BinaryOp::LEFT_SHIFT ||
                    expr.getOp() == BinaryOp::RIGHT_SHIFT) {
                    
                    lhs = builder.CreateFPToSI(lhs, Type::getInt64Ty(context.getContext()));
                    rhs = builder.CreateFPToSI(rhs, Type::getInt64Ty(context.getContext()));
                } else {
                    throw std::runtime_error("Unknown binary operator for floats");
                }
                break;
        }
    }

    if (lhs->getType()->isIntegerTy(1) && rhs->getType()->isIntegerTy(1)) {
        switch (expr.getOp()) {
            case BinaryOp::BITWISE_AND: 
            case BinaryOp::LOGICAL_AND: 
                return builder.CreateAnd(lhs, rhs, "andtmp");
            case BinaryOp::BITWISE_OR: 
            case BinaryOp::LOGICAL_OR: 
                return builder.CreateOr(lhs, rhs, "ortmp");
            case BinaryOp::BITWISE_XOR: 
                return builder.CreateXor(lhs, rhs, "xortmp");
            case BinaryOp::EQUAL: 
                return builder.CreateICmpEQ(lhs, rhs, "eqtmp");
            case BinaryOp::NOT_EQUAL: 
                return builder.CreateICmpNE(lhs, rhs, "netmp");
            default:
                break;
        }
    }

    if (lhs->getType() != rhs->getType()) {
        if (lhs->getType()->getIntegerBitWidth() < rhs->getType()->getIntegerBitWidth()) {
            lhs = builder.CreateSExt(lhs, rhs->getType());
        } else {
            rhs = builder.CreateSExt(rhs, lhs->getType());
        }
    }

    llvm::Type* resultType = lhs->getType();

    switch (expr.getOp()) {
        case BinaryOp::ADD: return builder.CreateAdd(lhs, rhs, "addtmp");
        case BinaryOp::SUBTRACT: return builder.CreateSub(lhs, rhs, "subtmp");
        case BinaryOp::MULTIPLY: return builder.CreateMul(lhs, rhs, "multmp");
        case BinaryOp::DIVIDE: 
            if (TypeBounds::isUnsignedType(AST::inferSourceType(lhs, context))) {
                return builder.CreateUDiv(lhs, rhs, "udivtmp");
            } else {
                return builder.CreateSDiv(lhs, rhs, "sdivtmp");
            }
        case BinaryOp::MODULUS:
            if (TypeBounds::isUnsignedType(AST::inferSourceType(lhs, context))) {
                return builder.CreateURem(lhs, rhs, "uremtmp");
            } else {
                return builder.CreateSRem(lhs, rhs, "sremtmp");
            }
        case BinaryOp::BITWISE_AND: return builder.CreateAnd(lhs, rhs, "andtmp");
        case BinaryOp::BITWISE_OR: return builder.CreateOr(lhs, rhs, "ortmp");
        case BinaryOp::BITWISE_XOR: return builder.CreateXor(lhs, rhs, "xortmp");
        case BinaryOp::LEFT_SHIFT: return builder.CreateShl(lhs, rhs, "shltmp");
        case BinaryOp::RIGHT_SHIFT:
            if (TypeBounds::isUnsignedType(AST::inferSourceType(lhs, context))) {
                return builder.CreateLShr(lhs, rhs, "lshrtmp");
            } else {
                return builder.CreateAShr(lhs, rhs, "ashrtmp");
            }
        case BinaryOp::GREATER: return builder.CreateICmpSGT(lhs, rhs, "gttmp");
        case BinaryOp::LESS: return builder.CreateICmpSLT(lhs, rhs, "lttmp");
        case BinaryOp::GREATER_EQUAL: return builder.CreateICmpSGE(lhs, rhs, "getmp");
        case BinaryOp::LESS_EQUAL: return builder.CreateICmpSLE(lhs, rhs, "letmp");
        case BinaryOp::EQUAL: return builder.CreateICmpEQ(lhs, rhs, "eqtmp");
        case BinaryOp::NOT_EQUAL: return builder.CreateICmpNE(lhs, rhs, "netmp");
        default: throw std::runtime_error("Unknown binary operator");
    }
}

llvm::Value* ExpressionCodeGen::codegenCall(CodeGen& context, CallExpr& expr) {
    auto& module = context.getModule();
    auto& builder = context.getBuilder();
    auto& llvmContext = context.getContext();

    auto& manager = StdLibManager::getInstance();
    
    std::string calleeName = expr.getCalleeExpr() ? "(member call)" : expr.getCallee();
    std::cout << "DEBUG: Generating call to: '" << calleeName 
              << "' with " << expr.getArgs().size() << " args\n";

    if (expr.getCalleeExpr()) {
        if (auto* memberAccess = dynamic_cast<MemberAccessExpr*>(expr.getCalleeExpr().get())) {
            std::cout << "DEBUG: Detected method call via member access" << std::endl;
            
            auto* object = memberAccess->getObject().get();
            const std::string& methodName = memberAccess->getMember();
            
            llvm::Value* selfPtr = nullptr;
            std::string structName;
            
            if (auto* varExpr = dynamic_cast<VariableExpr*>(object)) {
                std::string varName = varExpr->getName();
                auto varType = context.lookupVariableType(varName);
                
                if (varType == VarType::STRUCT) {
                    selfPtr = context.lookupVariable(varName);
                    
                    if (auto* arg = llvm::dyn_cast<llvm::Argument>(selfPtr)) {
                        if (arg->getType()->isPointerTy()) {
                            structName = context.getVariableStructName(varName);
                            std::cout << "DEBUG: Method call on function parameter '" << varName 
                                      << "' (struct: " << structName << ")" << std::endl;
                        }
                    }
                    else if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(selfPtr)) {
                        llvm::Type* allocatedType = alloca->getAllocatedType();
                        if (allocatedType->isStructTy()) {
                            llvm::StructType* structType = llvm::cast<llvm::StructType>(allocatedType);
                            structName = structType->getName().str();
                        }
                    } else if (auto* globalVar = llvm::dyn_cast<llvm::GlobalVariable>(selfPtr)) {
                        llvm::Type* valueType = globalVar->getValueType();
                        if (valueType->isStructTy()) {
                            llvm::StructType* structType = llvm::cast<llvm::StructType>(valueType);
                            structName = structType->getName().str();
                        }
                    }
                }
            }
            
            if (!structName.empty() && selfPtr) {
                std::string mangledMethodName = structName + "." + methodName;
                auto methodFunc = module.getFunction(mangledMethodName);
                
                if (methodFunc) {
                    std::cout << "DEBUG: Calling method '" << mangledMethodName << "'" << std::endl;
                    
                    std::vector<llvm::Value*> args;
                    
                    args.push_back(selfPtr);
                    std::cout << "DEBUG: Added self pointer to method call" << std::endl;
                    
                    unsigned argIdx = 0;
                    for (auto& argExpr : expr.getArgs()) {
                        llvm::Value* argValue = nullptr;

                        auto paramIter = methodFunc->arg_begin();
                        std::advance(paramIter, argIdx + 1);
                        
                        bool shouldPassAsPointer = false;
                        if (paramIter != methodFunc->arg_end()) {
                            llvm::Type* expectedType = paramIter->getType();
                            shouldPassAsPointer = expectedType->isPointerTy();
                            std::cout << "DEBUG: Argument " << argIdx << " expected type is pointer: " 
                                      << shouldPassAsPointer << std::endl;
                        }
                        
                        if (shouldPassAsPointer) {
                            if (auto* varExpr = dynamic_cast<VariableExpr*>(argExpr.get())) {
                                std::cout << "DEBUG: Argument " << argIdx << " is a VariableExpr: " 
                                          << varExpr->getName() << std::endl;
                                
                                auto varPtr = context.lookupVariable(varExpr->getName());
                                auto varType = context.lookupVariableType(varExpr->getName());
                                
                                std::cout << "DEBUG: Variable type is: " << static_cast<int>(varType) << std::endl;
                                
                                if (varPtr && varType == VarType::STRUCT) {
                                    argValue = varPtr;
                                    std::cout << "DEBUG: Passing struct pointer directly for argument '" 
                                              << varExpr->getName() << "'" << std::endl;
                                } else {
                                    std::cout << "DEBUG: Not passing as pointer - varPtr: " << (varPtr != nullptr) 
                                              << ", is STRUCT: " << (varType == VarType::STRUCT) << std::endl;
                                }
                            } else {
                                std::cout << "DEBUG: Argument " << argIdx << " is NOT a VariableExpr" << std::endl;
                            }
                        }
                        
                        if (!argValue) {
                            std::cout << "DEBUG: Calling codegen() for argument " << argIdx << std::endl;
                            argValue = argExpr->codegen(context);
                        }
                        
                        args.push_back(argValue);
                        argIdx++;
                    }
                    
                    std::cout << "DEBUG: Function '" << mangledMethodName << "' expects " 
                              << methodFunc->arg_size() << " arguments, got " << args.size() << std::endl;
                    
                    unsigned i = 0;
                    for (auto& arg : methodFunc->args()) {
                        std::cout << "  Param " << i << ": " << arg.getName().str() << " - ";
                        arg.getType()->print(llvm::errs());
                        if (i < args.size()) {
                            std::cout << " | Arg " << i << ": ";
                            args[i]->getType()->print(llvm::errs());
                        }
                        std::cout << std::endl;
                        i++;
                    }

                    if (args.size() != methodFunc->arg_size()) {
                        std::string errorMsg = "Incorrect number of arguments passed to called function!\n";
                        errorMsg += "  Function: " + mangledMethodName + "\n";
                        errorMsg += "  Expected: " + std::to_string(methodFunc->arg_size()) + "\n";
                        errorMsg += "  Got: " + std::to_string(args.size()) + "\n";
                        errorMsg += "  Args: ";
                        for (size_t i = 0; i < args.size(); ++i) {
                            std::string typeStr;
                            llvm::raw_string_ostream rso(typeStr);
                            args[i]->getType()->print(rso);
                            errorMsg += typeStr;
                            if (i < args.size() - 1) errorMsg += ", ";
                        }
                        throw std::runtime_error(errorMsg);
                    }
                    
                    return builder.CreateCall(methodFunc, args);
                }
            }
        }

        auto calleeValue = expr.getCalleeExpr()->codegen(context);
        
        bool isMathFunction = false;
        std::string funcName;
        
        if (auto* func = dyn_cast<Function>(calleeValue)) {
            funcName = func->getName().str();
            isMathFunction = (funcName.find("math_") == 0);
        }
        
        bool isPrintCall = false;
        bool isPrintlnCall = false;
        
        if (auto* func = dyn_cast<Function>(calleeValue)) {
            std::string funcName = func->getName().str();
            isPrintCall = (funcName == "io_print_str" || funcName == "print");
            isPrintlnCall = (funcName == "io_println_str" || funcName == "println");
        }

        bool isReadIntCall = false;
        if (auto* func = dyn_cast<Function>(calleeValue)) {
            std::string funcName = func->getName().str();
            isReadIntCall = (funcName == "io_read_int" || funcName == "read_int");
        }
        
        std::vector<llvm::Value*> args;
        for (auto& argExpr : expr.getArgs()) {
            auto argValue = argExpr->codegen(context);

            if (isMathFunction) {
                if (auto* func = dyn_cast<Function>(calleeValue)) {
                    auto funcType = func->getFunctionType();
                    size_t argIndex = args.size();
                    
                    if (argIndex < funcType->getNumParams()) {
                        llvm::Type* expectedType = funcType->getParamType(argIndex);
                        
                        if (expectedType->isFloatTy() && argValue->getType()->isIntegerTy()) {
                            std::cout << "DEBUG: Converting integer to float for math function argument" << std::endl;
                            argValue = builder.CreateSIToFP(argValue, Type::getFloatTy(llvmContext));
                        }
                        else if (expectedType->isFloatTy() && argValue->getType()->isDoubleTy()) {
                            argValue = builder.CreateFPTrunc(argValue, Type::getFloatTy(llvmContext));
                        }
                    }
                }
            }
            
            if ((isPrintlnCall || isPrintCall) && !argValue->getType()->isPointerTy()) {
                argValue = AST::convertToString(context, argValue);
            }
            
            args.push_back(argValue);
        }

        if (auto* func = dyn_cast<Function>(calleeValue)) {
            if (!func->getParent()) {
                throw std::runtime_error("Function not properly initialized");
            }
            
            auto funcType = func->getFunctionType();
            for (size_t i = 0; i < args.size() && i < funcType->getNumParams(); ++i) {
                llvm::Type* expectedType = funcType->getParamType(i);
                llvm::Type* actualType = args[i]->getType();
                
                if (actualType != expectedType) {
                    std::cout << "DEBUG: Type mismatch in argument " << i 
                              << ": expected ";
                    expectedType->print(llvm::errs());
                    std::cout << ", got ";
                    actualType->print(llvm::errs());
                    std::cout << std::endl;
                    
                    if (expectedType->isFloatTy() && actualType->isIntegerTy()) {
                        args[i] = builder.CreateSIToFP(args[i], Type::getFloatTy(llvmContext));
                        std::cout << "DEBUG: Converted integer to float" << std::endl;
                    } else if (expectedType->isFloatTy() && actualType->isDoubleTy()) {
                        args[i] = builder.CreateFPTrunc(args[i], Type::getFloatTy(llvmContext));
                        std::cout << "DEBUG: Converted double to float" << std::endl;
                    } else if (expectedType->isIntegerTy() && actualType->isFloatTy()) {
                        args[i] = builder.CreateFPToSI(args[i], expectedType);
                        std::cout << "DEBUG: Converted float to integer" << std::endl;
                    } else {
                        std::string expectedTypeStr;
                        llvm::raw_string_ostream expectedOS(expectedTypeStr);
                        expectedType->print(expectedOS);
                        
                        std::string actualTypeStr;
                        llvm::raw_string_ostream actualOS(actualTypeStr);
                        actualType->print(actualOS);
                        
                        throw std::runtime_error("Type mismatch in argument " + std::to_string(i) + 
                                               " for function '" + funcName + "': expected " + 
                                               expectedTypeStr + ", got " + actualTypeStr);
                    }
                }
            }
            
            FunctionCallee callee(func->getFunctionType(), func);
            llvm::Value* callResult = builder.CreateCall(callee, args);
            
            if (isReadIntCall) {
                std::string targetType = context.getCurrentTargetType();
                if (!targetType.empty()) {
                    callResult = addSimpleBoundsChecking(context, callResult, targetType);
                }
            }
            
            return callResult;
        } else {
            if (calleeValue->getType()->isPointerTy()) {
                auto funcType = FunctionType::get(
                    Type::getVoidTy(llvmContext),
                    {PointerType::get(Type::getInt8Ty(llvmContext), 0)},
                    false
                );
                FunctionCallee callee(funcType, calleeValue);
                return builder.CreateCall(callee, args);
            } else {
                throw std::runtime_error("Expected function pointer for member function call");
            }
        }
    }
    
    if (!expr.getCalleeExpr()) {
        if (expr.getCallee().empty()) {
            throw std::runtime_error("Empty function name in call expression");
        }
        
        std::string functionName = expr.getCallee();

        auto functionHandler = manager.findFunctionHandler(functionName, expr.getArgs().size());
        if (functionHandler) {
            std::cout << "DEBUG: Using function handler for: " << functionName << std::endl;
            llvm::Value* callResult = functionHandler->generateCall(context, expr);
            
            if (functionName == "read_int") {
                std::string targetType = context.getCurrentTargetType();
                if (!targetType.empty()) {
                    callResult = addSimpleBoundsChecking(context, callResult, targetType);
                }
            }
            
            return callResult;
        } else {
            std::cout << "DEBUG: No function handler found for: " << functionName << std::endl;
        }

        auto func = module.getFunction(functionName);
        if (!func) {
            std::cout << "DEBUG: Function '" << functionName << "' not found in module. Available functions:\n";
            for (auto& f : module) {
                std::cout << "  - " << f.getName().str() << std::endl;
            }
            throw std::runtime_error("Unknown function: " + functionName);
        }

        if (func->arg_size() != expr.getArgs().size()) {
            throw std::runtime_error("Function '" + functionName + "' expects " + 
                                   std::to_string(func->arg_size()) + " arguments, but got " + 
                                   std::to_string(expr.getArgs().size()));
        }
        
        std::vector<llvm::Value*> args;
        unsigned argIdx = 0;
        for (auto& argExpr : expr.getArgs()) {
            auto argValue = argExpr->codegen(context);
            if (!argValue) {
                throw std::runtime_error("Failed to generate argument " + std::to_string(argIdx) + 
                                       " for function " + functionName);
            }
            
            auto paramIter = func->arg_begin();
            std::advance(paramIter, argIdx);
            llvm::Type* expectedType = paramIter->getType();

            if (argValue->getType() != expectedType) {
                if (expectedType->isIntegerTy() && argValue->getType()->isIntegerTy()) {
                    unsigned expectedBits = expectedType->getIntegerBitWidth();
                    unsigned actualBits = argValue->getType()->getIntegerBitWidth();
                    
                    if (actualBits > expectedBits) {
                        argValue = builder.CreateTrunc(argValue, expectedType);
                    } else if (actualBits < expectedBits) {
                        VarType sourceType = AST::inferSourceType(argValue, context);
                        if (TypeBounds::isUnsignedType(sourceType)) {
                            argValue = builder.CreateZExt(argValue, expectedType);
                        } else {
                            argValue = builder.CreateSExt(argValue, expectedType);
                        }
                    }
                }
                else if (expectedType->isFPOrFPVectorTy() && argValue->getType()->isFPOrFPVectorTy()) {
                    if (expectedType->isFloatTy() && argValue->getType()->isDoubleTy()) {
                        argValue = builder.CreateFPTrunc(argValue, expectedType);
                    } else if (expectedType->isDoubleTy() && argValue->getType()->isFloatTy()) {
                        argValue = builder.CreateFPExt(argValue, expectedType);
                    }
                }
                else if (expectedType->isFPOrFPVectorTy() && argValue->getType()->isIntegerTy()) {
                    argValue = builder.CreateSIToFP(argValue, expectedType);
                }
                else if (expectedType->isIntegerTy() && argValue->getType()->isFPOrFPVectorTy()) {
                    argValue = builder.CreateFPToSI(argValue, expectedType);
                }
                else if (expectedType->isIntegerTy(1) && argValue->getType()->isIntegerTy()) {
                    argValue = builder.CreateICmpNE(argValue, ConstantInt::get(argValue->getType(), 0));
                }
                else if (expectedType->isIntegerTy() && argValue->getType()->isIntegerTy(1)) {
                    argValue = builder.CreateZExt(argValue, expectedType);
                }
                else {
                    std::string expectedTypeStr;
                    llvm::raw_string_ostream expectedOS(expectedTypeStr);
                    expectedType->print(expectedOS);
                    
                    std::string actualTypeStr;
                    llvm::raw_string_ostream actualOS(actualTypeStr);
                    argValue->getType()->print(actualOS);
                    
                    throw std::runtime_error("Type mismatch in argument " + std::to_string(argIdx) + 
                                           " for function '" + functionName + "': expected " + 
                                           expectedTypeStr + ", got " + actualTypeStr);
                }
            }
            
            args.push_back(argValue);
            argIdx++;
        }
        
        std::cout << "DEBUG: Creating call to function: " << functionName << std::endl;
        llvm::Value* callResult = builder.CreateCall(func, args);
        
        if (functionName == "read_int") {
            std::string targetType = context.getCurrentTargetType();
            if (!targetType.empty()) {
                callResult = addSimpleBoundsChecking(context, callResult, targetType);
            }
        }
        
        return callResult;
    }
    
    throw std::runtime_error("Invalid call expression");
}

llvm::Value* ExpressionCodeGen::addSimpleBoundsChecking(CodeGen& context, llvm::Value* value, const std::string& targetType) {
    auto& builder = context.getBuilder();
    auto& llvmContext = context.getContext();
    
    VarType varType = TypeBounds::stringToType(targetType);
    if (varType == VarType::VOID) {
        std::cout << "DEBUG: Unknown target type for bounds checking: " << targetType << std::endl;
        return value;
    }
    
    auto bounds = TypeBounds::getBounds(varType);
    if (!bounds.has_value()) {
        std::cout << "DEBUG: No bounds available for type: " << targetType << std::endl;
        return value;
    }
    
    auto [minVal, maxVal] = bounds.value();
    
    llvm::Value* minBound = llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvmContext), minVal);
    llvm::Value* maxBound = llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvmContext), maxVal);
    
    llvm::Value* isGeMin;
    llvm::Value* isLeMax;
    
    if (TypeBounds::isUnsignedType(varType)) {
        isGeMin = builder.CreateICmpUGE(value, minBound, "bounds_uge_min");
        isLeMax = builder.CreateICmpULE(value, maxBound, "bounds_ule_max");
    } else {
        isGeMin = builder.CreateICmpSGE(value, minBound, "bounds_sge_min");
        isLeMax = builder.CreateICmpSLE(value, maxBound, "bounds_sle_max");
    }
    
    llvm::Value* isInBounds = builder.CreateAnd(isGeMin, isLeMax, "bounds_check");
    
    llvm::Function* currentFunc = builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* errorBlock = llvm::BasicBlock::Create(llvmContext, "bounds_error", currentFunc);
    llvm::BasicBlock* continueBlock = llvm::BasicBlock::Create(llvmContext, "bounds_ok", currentFunc);
    
    builder.CreateCondBr(isInBounds, continueBlock, errorBlock);
    
    builder.SetInsertPoint(errorBlock);
    {
        std::string errorMsg = "Error: value %lld out of bounds for " + targetType + 
                              " (must be between " + std::to_string(minVal) + 
                              " and " + std::to_string(maxVal) + ")\n";
        llvm::Value* errorStr = builder.CreateGlobalStringPtr(errorMsg);
        
        auto& module = context.getModule();
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

        builder.CreateCall(fprintfFunc, {stderrVal, errorStr, value});
        
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

llvm::Value* ExpressionCodeGen::codegenBoolean(CodeGen& context, BooleanExpr& expr) {
    auto& builder = context.getBuilder();
    return ConstantInt::get(Type::getInt1Ty(context.getContext()), expr.getValue() ? 1 : 0);
}

llvm::Value* ExpressionCodeGen::codegenCast(CodeGen& context, CastExpr& expr) {
    auto value = expr.getExpr()->codegen(context);
    auto& builder = context.getBuilder();
    auto& llvmContext = context.getContext();
    
    VarType targetType = expr.getTargetType();
    
    VarType sourceType = AST::inferSourceType(value, context);
    if (!TypeBounds::isCastValid(sourceType, targetType)) {
        throw std::runtime_error("Invalid cast from " + TypeBounds::getTypeName(sourceType) + 
                               " to " + TypeBounds::getTypeName(targetType));
    }
    
    llvm::Type* sourceLLVMType = value->getType();
    llvm::Type* targetLLVMType = context.getLLVMType(targetType);
    
    if (!targetLLVMType) {
        throw std::runtime_error("Unknown target type in cast");
    }
    
    if (sourceLLVMType == targetLLVMType) {
        return value;
    }

    if (sourceLLVMType->isIntegerTy() && targetLLVMType->isIntegerTy()) {
        unsigned sourceBits = sourceLLVMType->getIntegerBitWidth();
        unsigned targetBits = targetLLVMType->getIntegerBitWidth();
        
        if (sourceBits < targetBits) {
            bool isUnsigned = TypeBounds::isUnsignedType(targetType);
            if (isUnsigned) {
                return builder.CreateZExt(value, targetLLVMType);
            } else {
                return builder.CreateSExt(value, targetLLVMType);
            }
        } else if (sourceBits > targetBits) {
            return builder.CreateTrunc(value, targetLLVMType);
        }
    }

    if (sourceLLVMType->isFPOrFPVectorTy() && targetLLVMType->isFPOrFPVectorTy()) {
        if (sourceLLVMType->isFloatTy() && targetLLVMType->isDoubleTy()) {
            return builder.CreateFPExt(value, targetLLVMType);
        } else if (sourceLLVMType->isDoubleTy() && targetLLVMType->isFloatTy()) {
            return builder.CreateFPTrunc(value, targetLLVMType);
        }
    }
    
    if (sourceLLVMType->isIntegerTy() && targetLLVMType->isFPOrFPVectorTy()) {
        bool isUnsigned = TypeBounds::isUnsignedType(sourceType);
        if (isUnsigned) {
            return builder.CreateUIToFP(value, targetLLVMType);
        } else {
            return builder.CreateSIToFP(value, targetLLVMType);
        }
    }
    
    if (sourceLLVMType->isFPOrFPVectorTy() && targetLLVMType->isIntegerTy()) {
        bool isUnsigned = TypeBounds::isUnsignedType(targetType);
        if (isUnsigned) {
            return builder.CreateFPToUI(value, targetLLVMType);
        } else {
            return builder.CreateFPToSI(value, targetLLVMType);
        }
    }
    
    if (sourceLLVMType->isIntegerTy(1) && targetLLVMType->isIntegerTy()) {
        return builder.CreateZExt(value, targetLLVMType);
    }
    
    if (sourceLLVMType->isIntegerTy() && targetLLVMType->isIntegerTy(1)) {
        return builder.CreateICmpNE(value, ConstantInt::get(sourceLLVMType, 0));
    }
    
    if (targetType == VarType::STRING) {
        return AST::convertToString(context, value);
    }
    
    if (sourceLLVMType->isPointerTy() && targetType != VarType::STRING) {
        throw std::runtime_error("Casting from string to non-string types not yet implemented");
    }
    
    throw std::runtime_error("Unsupported cast operation from " + 
                           TypeBounds::getTypeName(sourceType) + " to " + 
                           TypeBounds::getTypeName(targetType));
}

llvm::Value* ExpressionCodeGen::codegenFormatString(CodeGen& context, FormatStringExpr& expr) {
    auto& module = context.getModule();
    auto& builder = context.getBuilder();
    auto& llvmContext = context.getContext();

    auto* i8Ptr = PointerType::get(Type::getInt8Ty(llvmContext), 0);
    auto* sizeT = Type::getInt64Ty(llvmContext);
    auto* i32 = Type::getInt32Ty(llvmContext);
    
    auto snprintfFunc = module.getFunction("snprintf");
    if (!snprintfFunc) {
        std::vector<Type*> snprintfParams = {i8Ptr, sizeT, i8Ptr};
        auto snprintfType = FunctionType::get(i32, snprintfParams, true);
        snprintfFunc = Function::Create(snprintfType, Function::ExternalLinkage, "snprintf", &module);
    }

    auto mallocFunc = module.getFunction("malloc");
    if (!mallocFunc) {
        auto mallocType = FunctionType::get(i8Ptr, {sizeT}, false);
        mallocFunc = Function::Create(mallocType, Function::ExternalLinkage, "malloc", &module);
    }

    std::string formatSpecifiers;
    size_t lastPos = 0;
    const std::string& formatStr = expr.getFormatStr();
    
    size_t pos = 0;
    while ((pos = formatStr.find('{', lastPos)) != std::string::npos) {
        if (pos > lastPos) {
            formatSpecifiers += formatStr.substr(lastPos, pos - lastPos);
        }
        
        size_t endPos = formatStr.find('}', pos);
        if (endPos == std::string::npos) {
            throw std::runtime_error("Unclosed '{' in format string");
        }
        
        formatSpecifiers += "%s";
        
        lastPos = endPos + 1;
    }
    
    if (lastPos < formatStr.length()) {
        formatSpecifiers += formatStr.substr(lastPos);
    }

    std::vector<llvm::Value*> stringArgs;
    for (auto& exprPtr : expr.getExpressions()) {
        auto exprValue = exprPtr->codegen(context);
        auto stringValue = AST::convertToString(context, exprValue);
        stringArgs.push_back(stringValue);
    }
    
    auto bufferSize = ConstantInt::get(sizeT, 256);
    auto buffer = builder.CreateCall(mallocFunc, {bufferSize});
    
    auto formatStrPtr = builder.CreateGlobalStringPtr(formatSpecifiers);
    
    std::vector<llvm::Value*> snprintfArgs;
    snprintfArgs.push_back(buffer);
    snprintfArgs.push_back(bufferSize);
    snprintfArgs.push_back(formatStrPtr);
    
    for (auto stringArg : stringArgs) {
        snprintfArgs.push_back(stringArg);
    }
    
    builder.CreateCall(snprintfFunc, snprintfArgs);
    
    return buffer;
}

llvm::Value* ExpressionCodeGen::codegenModule(CodeGen& context, ModuleExpr& expr) {
    const std::string& moduleName = expr.getModuleName();
    
    if (moduleName == "std") {
        auto module = context.lookupVariable("std");
        if (!module) {
            throw std::runtime_error("Standard library module not found");
        }
        return module;
    }
    
    throw std::runtime_error("Unknown module: " + moduleName);
}

llvm::Value* ExpressionCodeGen::codegenMemberAccess(CodeGen& context, MemberAccessExpr& expr) {
    const std::string& member = expr.getMember();

    std::cout << "DEBUG codegenMemberAccess: Accessing member '" << member << "'" << std::endl;

    if (auto* varExpr = dynamic_cast<AST::VariableExpr*>(expr.getObject().get())) {
        std::string varName = varExpr->getName();
        auto varType = context.lookupVariableType(varName);
        
        std::cout << "DEBUG: Member access on variable '" << varName << "' with type " << static_cast<int>(varType) << std::endl;
        
        if (varType == AST::VarType::STRUCT) {
            auto var = context.lookupVariable(varName);
            if (!var) {
                throw std::runtime_error("Unknown variable: " + varName);
            }
            
            std::string structName;
            llvm::StructType* structType = nullptr;
            
            if (auto* arg = llvm::dyn_cast<llvm::Argument>(var)) {
                structName = context.getVariableStructName(varName);
                
                if (!structName.empty()) {
                    structType = llvm::dyn_cast<llvm::StructType>(context.getStructType(structName));
                }
                
                if (!structType) {
                    throw std::runtime_error("Could not determine struct type for parameter: " + varName);
                }
                
                std::cout << "DEBUG: Function parameter struct member access on struct '" << structName << "'" << std::endl;
                
                int fieldIndex = context.getStructFieldIndex(structName, member);
                if (fieldIndex != -1) {
                    auto& builder = context.getBuilder();
                    
                    llvm::Value* fieldPtr = builder.CreateStructGEP(
                        structType, 
                        var,
                        fieldIndex, 
                        member
                    );

                    llvm::Type* fieldType = structType->getElementType(fieldIndex);
                    
                    return builder.CreateLoad(fieldType, fieldPtr, member);
                }
                
                std::string methodName = structName + "." + member;
                auto methodFunc = context.getModule().getFunction(methodName);
                if (methodFunc) {
                    return methodFunc;
                }
                
                throw std::runtime_error("Unknown field or method '" + member + "' in struct '" + structName + "'");
            }
            else if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(var)) {
                llvm::Type* allocatedType = alloca->getAllocatedType();
                if (allocatedType->isStructTy()) {
                    structType = llvm::cast<llvm::StructType>(allocatedType);
                    structName = structType->getName().str();
                    
                    std::cout << "DEBUG: Struct member access on struct '" << structName << "'" << std::endl;
                    
                    int fieldIndex = context.getStructFieldIndex(structName, member);
                    if (fieldIndex != -1) {
                        auto& builder = context.getBuilder();
                        
                        llvm::Value* fieldPtr = builder.CreateStructGEP(structType, var, fieldIndex, member);
                        llvm::Type* fieldType = structType->getElementType(fieldIndex);
                        return builder.CreateLoad(fieldType, fieldPtr, member);
                    }
                    
                    std::string methodName = structName + "." + member;
                    auto methodFunc = context.getModule().getFunction(methodName);
                    if (methodFunc) {
                        if (methodFunc->arg_size() > 0) {
                            auto firstParam = methodFunc->arg_begin();
                            if (firstParam->getType()->isPointerTy() && 
                                firstParam->getName() == "self") {
                                std::cout << "DEBUG: Found method '" << methodName << "' with self parameter" << std::endl;
                                return methodFunc;
                            }
                        }
                    }
                    
                    throw std::runtime_error("Unknown field or method '" + member + "' in struct '" + structName + "'");
                }
            }
            else if (auto* globalVar = llvm::dyn_cast<llvm::GlobalVariable>(var)) {
                llvm::Type* valueType = globalVar->getValueType();
                if (valueType->isStructTy()) {
                    structType = llvm::cast<llvm::StructType>(valueType);
                    structName = structType->getName().str();
                    
                    std::cout << "DEBUG: Global struct member access on struct '" << structName << "'" << std::endl;

                    int fieldIndex = context.getStructFieldIndex(structName, member);
                    if (fieldIndex != -1) {
                        auto& builder = context.getBuilder();
                        
                        llvm::Value* fieldPtr = builder.CreateStructGEP(structType, var, fieldIndex, member);
                        llvm::Type* fieldType = structType->getElementType(fieldIndex);
                        return builder.CreateLoad(fieldType, fieldPtr, member);
                    }
                    
                    std::string methodName = structName + "." + member;
                    auto methodFunc = context.getModule().getFunction(methodName);
                    if (methodFunc) {
                        return methodFunc;
                    }
                    
                    throw std::runtime_error("Unknown field or method '" + member + "' in struct '" + structName + "'");
                }
            }
        }

        std::string actualModuleName = context.resolveModuleAlias(varName);
        if (!actualModuleName.empty()) {
            return handleModuleMemberAccess(context, actualModuleName, member);
        }
        
        if (varType == AST::VarType::MODULE) {
            std::string resolvedName = context.getModuleIdentity(varName);
            if (!resolvedName.empty()) {
                return handleModuleMemberAccess(context, resolvedName, member);
            }
            return handleModuleMemberAccess(context, varName, member);
        }
    }

    auto object = expr.getObject()->codegen(context);

    if (object->getType()->isStructTy()) {
        llvm::StructType* structType = llvm::cast<llvm::StructType>(object->getType());
        std::string structName = structType->getName().str();
        
        std::cout << "DEBUG: Loaded struct value member access on struct '" << structName << "'" << std::endl;
        
        int fieldIndex = context.getStructFieldIndex(structName, member);
        if (fieldIndex != -1) {
            auto& builder = context.getBuilder();
            
            return builder.CreateExtractValue(object, fieldIndex, member);
        }
        
        std::string methodName = structName + "." + member;
        auto methodFunc = context.getModule().getFunction(methodName);
        if (methodFunc) {
            return methodFunc;
        }
        
        throw std::runtime_error("Unknown field or method '" + member + "' in struct '" + structName + "'");
    }
    
    if (auto* globalVar = llvm::dyn_cast<llvm::GlobalVariable>(object)) {
        std::string moduleName = globalVar->getName().str();
        
        if (context.lookupVariable(moduleName)) {
            auto varType = context.lookupVariableType(moduleName);
            if (varType == AST::VarType::MODULE) {
                std::string actualModuleName = context.getModuleIdentity(moduleName);
                if (!actualModuleName.empty()) {
                    return handleModuleMemberAccess(context, actualModuleName, member);
                }
                return handleModuleMemberAccess(context, moduleName, member);
            }
        }
    }
    
    if (auto* memberAccess = dynamic_cast<AST::MemberAccessExpr*>(expr.getObject().get())) {
        auto baseObject = memberAccess->codegen(context);
        
        if (auto* globalVar = llvm::dyn_cast<llvm::GlobalVariable>(baseObject)) {
            std::string baseName = globalVar->getName().str();
            return handleModuleMemberAccess(context, baseName, member);
        }
    }
    
    throw std::runtime_error("Unknown member access: " + member);
}

std::string ExpressionCodeGen::extractModuleName(CodeGen& context, const std::string& varName) {
    std::string actualName = context.getModuleIdentity(varName);
    if (!actualName.empty()) {
        return actualName;
    }

    return varName;
}

llvm::Value* ExpressionCodeGen::handleModuleMemberAccess(CodeGen& context, const std::string& moduleName, const std::string& member) {
    std::string actualModuleName = moduleName;

    auto& manager = StdLibManager::getInstance();
    
    auto moduleHandler = manager.findModuleHandler(actualModuleName);
    
    if (!moduleHandler) {
        size_t lastDotPos = actualModuleName.find_last_of('.');
        if (lastDotPos != std::string::npos) {
            std::string shortName = actualModuleName.substr(lastDotPos + 1);
            moduleHandler = manager.findModuleHandler(shortName);
            if (moduleHandler) {
                actualModuleName = shortName;
            }
        }
    }
    
    if (moduleHandler) {
        return moduleHandler->getMember(context, actualModuleName, member);
    }
    
    throw std::runtime_error("Unknown member '" + member + "' in module '" + moduleName + "'");
}

llvm::Value* ExpressionCodeGen::codegenEnumValue(CodeGen& context, EnumValueExpr& expr) {
    std::string fullName = expr.getEnumName() + "." + expr.getMemberName();
    auto enumValue = context.lookupVariable(fullName);
    
    if (!enumValue) {
        throw std::runtime_error("Unknown enum value: " + fullName);
    }
    
    auto& builder = context.getBuilder();
    return builder.CreateLoad(builder.getInt32Ty(), enumValue, fullName.c_str());
}

llvm::Value* ExpressionCodeGen::codegenStructLiteral(CodeGen& context, StructLiteralExpr& expr) {
    auto& builder = context.getBuilder();
    auto& llvmContext = context.getContext();
    
    std::string structName = expr.getStructName();
    llvm::Type* structType = context.getStructType(structName);
    if (!structType) {
        throw std::runtime_error("Unknown struct type: " + structName);
    }
    
    if (!structType->isStructTy()) {
        throw std::runtime_error("Expected struct type for: " + structName);
    }
    llvm::StructType* structTy = llvm::cast<llvm::StructType>(structType);
    
    const auto& structFields = context.getStructFields(structName);
    if (structFields.empty()) {
        throw std::runtime_error("No field information found for struct: " + structName);
    }
    
    std::cout << "DEBUG: Generating struct literal for '" << structName << "' with " 
              << expr.getFields().size() << " provided fields\n";
    
    llvm::AllocaInst* alloca = builder.CreateAlloca(structTy, nullptr, structName + "_tmp");
    
    std::unordered_map<std::string, llvm::Value*> providedFields;
    for (const auto& field : expr.getFields()) {
        std::cout << "DEBUG: Processing field '" << field.first << "' in struct literal\n";
        providedFields[field.first] = field.second->codegen(context);
    }
    
    for (size_t i = 0; i < structFields.size(); i++) {
        const auto& fieldInfo = structFields[i];
        const std::string& fieldName = fieldInfo.first;
        VarType fieldType = fieldInfo.second;
        
        llvm::Value* fieldPtr = builder.CreateStructGEP(structTy, alloca, i, fieldName);
        llvm::Type* expectedFieldType = structTy->getElementType(i);
        
        std::cout << "DEBUG: Initializing field '" << fieldName << "' type: ";
        expectedFieldType->print(llvm::errs());
        llvm::errs() << "\n";
        
        auto providedIt = providedFields.find(fieldName);
        if (providedIt != providedFields.end()) {
            llvm::Value* providedValue = providedIt->second;
            
            std::cout << "DEBUG: Field '" << fieldName << "' provided with value type: ";
            providedValue->getType()->print(llvm::errs());
            llvm::errs() << "\n";
            
            if (providedValue->getType() != expectedFieldType) {
                if (expectedFieldType->isFPOrFPVectorTy() && providedValue->getType()->isIntegerTy()) {
                    std::string fieldTypeStr = expectedFieldType->isFloatTy() ? "float32" : "float64";
                    throw std::runtime_error(
                        "Type mismatch for field '" + fieldName + "' in struct '" + structName + "': " +
                        "expected " + fieldTypeStr + ", but got an integer. " +
                        "Use a float literal like 30.0 instead of 30"
                    );
                }
                else if (expectedFieldType->isIntegerTy() && providedValue->getType()->isFPOrFPVectorTy()) {
                    throw std::runtime_error(
                        "Type mismatch for field '" + fieldName + "' in struct '" + structName + "': " +
                        "expected integer type, but got a float"
                    );
                }
                else if (expectedFieldType->isIntegerTy() && providedValue->getType()->isIntegerTy()) {
                    unsigned expectedBits = expectedFieldType->getIntegerBitWidth();
                    unsigned actualBits = providedValue->getType()->getIntegerBitWidth();
                    
                    if (actualBits > expectedBits) {
                        providedValue = builder.CreateTrunc(providedValue, expectedFieldType);
                    } else if (actualBits < expectedBits) {
                        if (TypeBounds::isUnsignedType(fieldType)) {
                            providedValue = builder.CreateZExt(providedValue, expectedFieldType);
                        } else {
                            providedValue = builder.CreateSExt(providedValue, expectedFieldType);
                        }
                    }
                }
                else if (expectedFieldType->isFPOrFPVectorTy() && providedValue->getType()->isFPOrFPVectorTy()) {
                    if (expectedFieldType->isFloatTy() && providedValue->getType()->isDoubleTy()) {
                        providedValue = builder.CreateFPTrunc(providedValue, expectedFieldType);
                    } else if (expectedFieldType->isDoubleTy() && providedValue->getType()->isFloatTy()) {
                        providedValue = builder.CreateFPExt(providedValue, expectedFieldType);
                    }
                }
                else {
                    std::string expectedTypeStr;
                    llvm::raw_string_ostream expectedOS(expectedTypeStr);
                    expectedFieldType->print(expectedOS);
                    
                    std::string actualTypeStr;
                    llvm::raw_string_ostream actualOS(actualTypeStr);
                    providedValue->getType()->print(actualOS);
                    
                    throw std::runtime_error(
                        "Type mismatch for field '" + fieldName + "' in struct '" + structName + "': " +
                        "expected " + expectedTypeStr + ", got " + actualTypeStr
                    );
                }
            }
            
            builder.CreateStore(providedValue, fieldPtr);
            std::cout << "DEBUG: Stored provided value for field '" << fieldName << "'\n";
        } else {
            llvm::Constant* defaultVal = context.getStructFieldDefault(structName, fieldName);
            if (defaultVal) {
                std::cout << "DEBUG: Using stored default value for field '" << fieldName << "'\n";
                builder.CreateStore(defaultVal, fieldPtr);
            } else {
                std::cout << "DEBUG: Field '" << fieldName << "' not provided and no default, using zero\n";
                llvm::Type* fieldLLVMType = structTy->getElementType(i);
                llvm::Constant* zeroVal = createDefaultValue(fieldLLVMType, fieldType);
                builder.CreateStore(zeroVal, fieldPtr);
            }
        }
    }
    
    llvm::Value* loadedStruct = builder.CreateLoad(structTy, alloca, structName + "_val");
    std::cout << "DEBUG: Struct literal generated, returning type: ";
    loadedStruct->getType()->print(llvm::errs());
    llvm::errs() << "\n";
    
    return loadedStruct;
}