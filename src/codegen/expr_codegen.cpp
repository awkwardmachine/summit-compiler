#include "expr_codegen.h"
#include "codegen.h"
#include <llvm/IR/Verifier.h>
#include "codegen/bounds.h"
#include <iostream>
#include <regex>
#include <vector>
#include <sstream>
#include <cstdint>
#include <climits>
#include <limits>

using namespace llvm;
using namespace AST;

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

llvm::Value* convertToString(CodeGen& context, llvm::Value* value) {
    auto& builder = context.getBuilder();
    auto& module = context.getModule();
    auto& llvmContext = context.getContext();
    
    if (value->getType()->isPointerTy()) {
        return value;
    }

    auto mallocFunc = module.getFunction("malloc");
    if (!mallocFunc) {
        auto* i8Ptr = PointerType::get(Type::getInt8Ty(llvmContext), 0);
        auto* sizeT = Type::getInt64Ty(llvmContext);
        auto mallocType = FunctionType::get(i8Ptr, {sizeT}, false);
        mallocFunc = Function::Create(mallocType, Function::ExternalLinkage, "malloc", &module);
    }
    
    auto sprintfFunc = module.getFunction("sprintf");
    if (!sprintfFunc) {
        auto* i8Ptr = PointerType::get(Type::getInt8Ty(llvmContext), 0);
        auto* i32 = Type::getInt32Ty(llvmContext);
        std::vector<Type*> paramTypes = {i8Ptr, i8Ptr};
        auto funcType = FunctionType::get(i32, paramTypes, true);
        sprintfFunc = Function::Create(funcType, Function::ExternalLinkage, "sprintf", &module);
    }

    const int BUFFER_SIZE = 64;
    auto* buffer = builder.CreateCall(mallocFunc, {ConstantInt::get(builder.getInt64Ty(), BUFFER_SIZE)});

    llvm::Value* formatStr;
    
    if (value->getType()->isFloatTy()) {
        formatStr = builder.CreateGlobalStringPtr("%.6f");
        value = builder.CreateFPExt(value, Type::getDoubleTy(llvmContext));
        std::vector<Value*> sprintfArgs = {buffer, formatStr, value};
        builder.CreateCall(sprintfFunc, sprintfArgs);
    }
    else if (value->getType()->isDoubleTy()) {
        formatStr = builder.CreateGlobalStringPtr("%.15lf");
        std::vector<Value*> sprintfArgs = {buffer, formatStr, value};
        builder.CreateCall(sprintfFunc, sprintfArgs);
    }
    else if (value->getType()->isIntegerTy(1)) {
        auto zero64 = builder.CreateZExt(value, Type::getInt64Ty(llvmContext));
        formatStr = builder.CreateGlobalStringPtr("%s");
        auto trueStr = builder.CreateGlobalStringPtr("true");
        auto falseStr = builder.CreateGlobalStringPtr("false");
        auto isTrue = builder.CreateICmpNE(zero64, ConstantInt::get(zero64->getType(), 0));
        auto boolStr = builder.CreateSelect(isTrue, trueStr, falseStr);
        
        auto strcpyFunc = module.getFunction("strcpy");
        if (!strcpyFunc) {
            auto* i8Ptr = PointerType::get(Type::getInt8Ty(llvmContext), 0);
            std::vector<Type*> paramTypes = {i8Ptr, i8Ptr};
            auto funcType = FunctionType::get(i8Ptr, paramTypes, false);
            strcpyFunc = Function::Create(funcType, Function::ExternalLinkage, "strcpy", &module);
        }
        builder.CreateCall(strcpyFunc, {buffer, boolStr});
    } 
    else {
        auto isLarge = builder.CreateICmpUGE(value, ConstantInt::get(value->getType(), 1ULL << 63));

        auto signedFormatStr = builder.CreateGlobalStringPtr("%ld");
        auto unsignedFormatStr = builder.CreateGlobalStringPtr("%lu");
        
        formatStr = builder.CreateSelect(isLarge, unsignedFormatStr, signedFormatStr);
        
        std::vector<Value*> sprintfArgs = {buffer, formatStr, value};
        builder.CreateCall(sprintfFunc, sprintfArgs);
    }
    
    return buffer;
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
    auto& namedValues = context.getNamedValues();
    auto var = namedValues[expr.getName()];
    if (!var) {
        throw std::runtime_error("Unknown variable: " + expr.getName());
    }
    
    auto& variableTypes = context.getVariableTypes();
    auto typeIt = variableTypes.find(expr.getName());
    
    auto& builder = context.getBuilder();

    if (typeIt != variableTypes.end() && typeIt->second == AST::VarType::STRING) {
        return builder.CreateLoad(
            llvm::PointerType::get(Type::getInt8Ty(context.getContext()), 0),
            var,
            expr.getName().c_str()
        );
    }

    if (typeIt != variableTypes.end() && 
        (typeIt->second == AST::VarType::BOOL || typeIt->second == AST::VarType::UINT0)) {
        return builder.CreateLoad(Type::getInt1Ty(context.getContext()), var, expr.getName().c_str());
    }

    if (typeIt != variableTypes.end() && 
        (typeIt->second == AST::VarType::FLOAT32 || typeIt->second == AST::VarType::FLOAT64)) {
        llvm::Type* loadType = context.getLLVMType(typeIt->second);
        return builder.CreateLoad(loadType, var, expr.getName().c_str());
    }

    bool isUnsigned = false;
    if (typeIt != variableTypes.end()) {
        auto varType = typeIt->second;
        isUnsigned = (varType == AST::VarType::UINT4 || varType == AST::VarType::UINT8 || 
                     varType == AST::VarType::UINT12 || varType == AST::VarType::UINT16 || 
                     varType == AST::VarType::UINT24 || varType == AST::VarType::UINT32 || 
                     varType == AST::VarType::UINT48 || varType == AST::VarType::UINT64);
    }

    llvm::Type* loadType = Type::getInt64Ty(context.getContext());
    if (typeIt != variableTypes.end()) {
        loadType = context.getLLVMType(typeIt->second);
    }
    
    llvm::Value* loadedValue = builder.CreateLoad(loadType, var, expr.getName().c_str());

    if (isUnsigned && loadedValue->getType()->getIntegerBitWidth() < 64) {
        loadedValue = builder.CreateZExt(loadedValue, Type::getInt64Ty(context.getContext()));
    }
    
    return loadedValue;
}

llvm::Value* ExpressionCodeGen::codegenBinary(CodeGen& context, BinaryExpr& expr) {
    auto lhs = expr.getLHS()->codegen(context);
    auto rhs = expr.getRHS()->codegen(context);
    auto& builder = context.getBuilder();

    if (expr.getOp() == BinaryOp::ADD) {
        bool lhsIsString = lhs->getType()->isPointerTy();
        bool rhsIsString = rhs->getType()->isPointerTy();
        
        if (lhsIsString || rhsIsString) {
            if (!lhsIsString && !isConvertibleToString(lhs)) {
                throw std::runtime_error("Left operand cannot be converted to string for concatenation");
            }
            if (!rhsIsString && !isConvertibleToString(rhs)) {
                throw std::runtime_error("Right operand cannot be converted to string for concatenation");
            }
            
            auto& module = context.getModule();
            auto& llvmContext = context.getContext();
            llvm::Value* lhsStr = lhsIsString ? lhs : convertToString(context, lhs);
            llvm::Value* rhsStr = rhsIsString ? rhs : convertToString(context, rhs);
            
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
                std::vector<Type*> paramTypes = {i8Ptr, i8Ptr};
                auto funcType = FunctionType::get(i8Ptr, paramTypes, false);
                strcpyFunc = Function::Create(funcType, Function::ExternalLinkage, "strcpy", &module);
            }
            
            auto strcatFunc = module.getFunction("strcat");
            if (!strcatFunc) {
                auto* i8Ptr = PointerType::get(Type::getInt8Ty(llvmContext), 0);
                std::vector<Type*> paramTypes = {i8Ptr, i8Ptr};
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

    if ((lhs->getType()->isPointerTy() || rhs->getType()->isPointerTy()) && expr.getOp() != BinaryOp::ADD) {
        throw std::runtime_error("Unsupported operation for string types");
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
            case BinaryOp::ADD: 
                return builder.CreateFAdd(lhs, rhs, "faddtmp");
            case BinaryOp::SUBTRACT: 
                return builder.CreateFSub(lhs, rhs, "fsubtmp");
            case BinaryOp::MULTIPLY: 
                return builder.CreateFMul(lhs, rhs, "fmultmp");
            case BinaryOp::DIVIDE: 
                return builder.CreateFDiv(lhs, rhs, "fdivtmp");
            default: 
                throw std::runtime_error("Unknown binary operator for floats");
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
            return builder.CreateSDiv(lhs, rhs, "divtmp");
        default: throw std::runtime_error("Unknown binary operator");
    }
}

std::string buildFormatSpecifiers(const std::string& formatStr) {
    std::string formatSpecifiers;
    
    size_t pos = 0;
    size_t lastPos = 0;
    
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
    
    return formatSpecifiers;
}

std::vector<std::string> extractExpressionStrings(const std::string& formatStr) {
    std::vector<std::string> expressions;
    
    size_t pos = 0;
    while ((pos = formatStr.find('{', pos)) != std::string::npos) {
        size_t endPos = formatStr.find('}', pos);
        if (endPos == std::string::npos) {
            throw std::runtime_error("Unclosed '{' in format string");
        }
        
        std::string exprStr = formatStr.substr(pos + 1, endPos - pos - 1);
        expressions.push_back(exprStr);
        
        pos = endPos + 1;
    }
    
    return expressions;
}

llvm::Value* ExpressionCodeGen::codegenCall(CodeGen& context, CallExpr& expr) {
    auto& module = context.getModule();
    auto& builder = context.getBuilder();
    
    if (expr.getCallee() == "@print" || expr.getCallee() == "@println") {
        if (expr.getArgs().size() < 1) {
            throw std::runtime_error("print/println expects at least one argument");
        }
        
        auto printfFunc = module.getFunction("printf");
        if (!printfFunc) {
            throw std::runtime_error("printf function not found");
        }
        
        auto firstArg = expr.getArgs()[0]->codegen(context);

        if (auto formatStrValue = dyn_cast<GlobalVariable>(firstArg)) {
            if (auto formatStrConstant = dyn_cast<ConstantDataSequential>(formatStrValue->getInitializer())) {
                std::string formatStr = formatStrConstant->getAsString().drop_back().str();
                
                if (formatStr.find('{') != std::string::npos) {
                    auto formatSpecifiers = buildFormatSpecifiers(formatStr);

                    if (expr.getCallee() == "@println") {
                        formatSpecifiers += "\n";
                    }
                    
                    auto exprStrings = extractExpressionStrings(formatStr);
                    
                    std::vector<Value*> args;
                    auto formatSpecifierStr = builder.CreateGlobalStringPtr(formatSpecifiers);
                    args.push_back(formatSpecifierStr);

                    for (const auto& exprStr : exprStrings) {
                        VariableExpr varExpr(exprStr);
                        auto argValue = varExpr.codegen(context);
                        auto stringValue = convertToString(context, argValue);
                        args.push_back(stringValue);
                    }
                    
                    return builder.CreateCall(printfFunc, args);
                } else {
                    std::vector<Value*> args;
                    
                    if (expr.getCallee() == "@println") {
                        auto formatWithNewline = formatStr + "\n";
                        auto formatStrPtr = builder.CreateGlobalStringPtr(formatWithNewline);
                        args.push_back(formatStrPtr);
                    } else {
                        auto formatStrPtr = builder.CreateGlobalStringPtr(formatStr);
                        args.push_back(formatStrPtr);
                    }
                    
                    return builder.CreateCall(printfFunc, args);
                }
            }
        }
        
        std::vector<Value*> args;

        std::string formatStr;
        for (size_t i = 0; i < expr.getArgs().size(); i++) {
            if (i > 0) formatStr += " ";
            formatStr += "%s";
        }

        if (expr.getCallee() == "@println") {
            formatStr += "\n";
        }
        
        auto formatStrPtr = builder.CreateGlobalStringPtr(formatStr);
        args.push_back(formatStrPtr);

        for (size_t i = 0; i < expr.getArgs().size(); i++) {
            auto argValue = expr.getArgs()[i]->codegen(context);
            auto stringValue = convertToString(context, argValue);
            args.push_back(stringValue);
        }
        
        return builder.CreateCall(printfFunc, args);
    }

    auto func = module.getFunction(expr.getCallee());
    if (!func) {
        throw std::runtime_error("Unknown function: " + expr.getCallee());
    }

    std::vector<Value*> args;
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
    
    VarType sourceType = inferSourceType(value, context);
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
        return convertToString(context, value);
    }
    
    if (sourceLLVMType->isPointerTy() && targetType != VarType::STRING) {
        throw std::runtime_error("Casting from string to non-string types not yet implemented");
    }
    
    throw std::runtime_error("Unsupported cast operation from " + 
                           TypeBounds::getTypeName(sourceType) + " to " + 
                           TypeBounds::getTypeName(targetType));
}