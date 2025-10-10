#include "expr_codegen.h"
#include "type_inference.h"
#include "string_conversions.h"
#include "format_utils.h"
#include "codegen/bounds.h"
#include <llvm/IR/Verifier.h>
#include <iostream>
#include <regex>
#include <vector>
#include <sstream>
#include <cstdint>
#include <climits>
#include <limits>

using namespace llvm;
using namespace AST;

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
    auto var = context.lookupVariable(expr.getName());
    if (!var) {
        throw std::runtime_error("Unknown variable: " + expr.getName());
    }
    
    auto varType = context.lookupVariableType(expr.getName());
    
    std::cout << "DEBUG codegenVariable: " << expr.getName() 
              << " type = " << static_cast<int>(varType) << std::endl;
    
    auto& builder = context.getBuilder();

    bool isUnsigned = TypeBounds::isUnsignedType(varType);

    llvm::Type* loadType = context.getLLVMType(varType);
    if (!loadType) {
        throw std::runtime_error("Unknown variable type for: " + expr.getName());
    }
    
    std::cout << "DEBUG: Load type = ";
    loadType->print(llvm::errs());
    std::cout << std::endl;
    
    llvm::Value* loadedValue = builder.CreateLoad(loadType, var, expr.getName().c_str());

    if (loadType->isIntegerTy() && loadType->getIntegerBitWidth() < 64) {
        std::cout << "DEBUG: Extending " << loadType->getIntegerBitWidth() 
                  << "-bit integer to 64 bits" << std::endl;
        if (isUnsigned) {
            loadedValue = builder.CreateZExt(loadedValue, Type::getInt64Ty(context.getContext()));
        } else {
            loadedValue = builder.CreateSExt(loadedValue, Type::getInt64Ty(context.getContext()));
        }
    }
    
    return loadedValue;
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
            if (!lhsIsString && !AST::isConvertibleToString(lhs)) {
                throw std::runtime_error("Left operand cannot be converted to string for concatenation");
            }
            if (!rhsIsString && !AST::isConvertibleToString(rhs)) {
                throw std::runtime_error("Right operand cannot be converted to string for concatenation");
            }
            
            auto& module = context.getModule();
            auto& llvmContext = context.getContext();
            llvm::Value* lhsStr = lhsIsString ? lhs : AST::convertToString(context, lhs);
            llvm::Value* rhsStr = rhsIsString ? rhs : AST::convertToString(context, rhs);
            
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
            
            auto lhsLen = builder.CreateCall(strlenFunc, {lhsStr});
            auto rhsLen = builder.CreateCall(strlenFunc, {rhsStr});
            auto totalLen = builder.CreateAdd(lhsLen, rhsLen);
            auto bufferSize = builder.CreateAdd(totalLen, ConstantInt::get(llvmContext, APInt(64, 1)));
            
            auto buffer = builder.CreateCall(mallocFunc, {bufferSize});
            builder.CreateCall(strcpyFunc, {buffer, lhsStr});
            builder.CreateCall(strcatFunc, {buffer, rhsStr});
            
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

    if (lhs->getType()->getIntegerBitWidth() == 1)
        lhs = builder.CreateZExt(lhs, Type::getInt64Ty(context.getContext()));
    if (rhs->getType()->getIntegerBitWidth() == 1)
        rhs = builder.CreateZExt(rhs, Type::getInt64Ty(context.getContext()));

    if (lhs->getType() != rhs->getType()) {
        if (lhs->getType()->getIntegerBitWidth() < rhs->getType()->getIntegerBitWidth()) {
            lhs = builder.CreateSExt(lhs, rhs->getType());
        } else {
            rhs = builder.CreateSExt(rhs, lhs->getType());
        }
    }

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

    if (expr.getCallee() == "@dec") {
        if (expr.getArgs().size() != 1) {
            throw std::runtime_error("@dec expects exactly one argument");
        }
        auto value = expr.getArgs()[0]->codegen(context);
        return AST::convertToDecimalString(context, value);
    }

    if (expr.getCallee() == "@bin") {
        if (expr.getArgs().size() != 1) {
            throw std::runtime_error("@bin expects exactly one argument");
        }
        auto value = expr.getArgs()[0]->codegen(context);
        return AST::convertToBinaryString(context, value);
    }
    
    if (expr.getCallee() == "@print" || expr.getCallee() == "@println") {
        if (expr.getArgs().size() < 1) {
            throw std::runtime_error("print/println expects at least one argument");
        }
        
        auto printfFunc = module.getFunction("printf");
        if (!printfFunc) {
            auto* i32 = Type::getInt32Ty(llvmContext);
            auto* i8Ptr = PointerType::get(Type::getInt8Ty(llvmContext), 0);
            auto printfType = FunctionType::get(i32, {i8Ptr}, true);
            printfFunc = Function::Create(printfType, Function::ExternalLinkage, "printf", &module);
        }
        
        if (auto* formatExpr = dynamic_cast<FormatStringExpr*>(expr.getArgs()[0].get())) {
            auto result = formatExpr->codegen(context);
            
            if (expr.getCallee() == "@println") {
                const std::string& formatStr = formatExpr->getFormatStr();
                if (!formatStr.empty() && formatStr.back() != '\n') {
                    auto newlineStr = builder.CreateGlobalStringPtr("\n");
                    builder.CreateCall(printfFunc, {newlineStr});
                }
            }
            
            return result;
        }

        std::vector<llvm::Value*> printfArgs;

        std::string formatStr;
        for (size_t i = 0; i < expr.getArgs().size(); i++) {
            if (i > 0) formatStr += " ";
            formatStr += "%s";
        }
        
        if (expr.getCallee() == "@println") {
            formatStr += "\n";
        }
        
        auto formatStrPtr = builder.CreateGlobalStringPtr(formatStr);
        printfArgs.push_back(formatStrPtr);
        
        for (size_t i = 0; i < expr.getArgs().size(); i++) {
            auto argValue = expr.getArgs()[i]->codegen(context);
            auto stringValue = AST::convertToString(context, argValue);
            printfArgs.push_back(stringValue);
        }
        
        return builder.CreateCall(printfFunc, printfArgs);
    }
    
    auto func = module.getFunction(expr.getCallee());
    if (!func) {
        throw std::runtime_error("Unknown function: " + expr.getCallee());
    }
    
    std::vector<llvm::Value*> args;
    for (auto& arg : expr.getArgs()) {
        args.push_back(arg->codegen(context));
    }
    
    return builder.CreateCall(func, args);
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
    
    auto printfFunc = module.getFunction("printf");
    if (!printfFunc) {
        auto* i32 = Type::getInt32Ty(llvmContext);
        auto* i8Ptr = PointerType::get(Type::getInt8Ty(llvmContext), 0);
        auto printfType = FunctionType::get(i32, {i8Ptr}, true);
        printfFunc = Function::Create(printfType, Function::ExternalLinkage, "printf", &module);
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

    std::vector<llvm::Value*> printfArgs;
    auto formatStrPtr = builder.CreateGlobalStringPtr(formatSpecifiers);
    printfArgs.push_back(formatStrPtr);
    
    for (auto& expr : expr.getExpressions()) {
        auto exprValue = expr->codegen(context);
        auto stringValue = AST::convertToString(context, exprValue);
        printfArgs.push_back(stringValue);
    }

    return builder.CreateCall(printfFunc, printfArgs);
}