#include "expr_codegen.h"
#include "codegen.h"
#include <llvm/IR/Verifier.h>
#include "codegen/bounds.h"
#include <iostream>

using namespace llvm;
using namespace AST;

// Add string codegen implementation
llvm::Value* ExpressionCodeGen::codegenString(CodeGen& context, StringExpr& expr) {
    auto& builder = context.getBuilder();
    const std::string& strValue = expr.getValue();
    
    // Create a global constant string
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
   
    int64_t int64Value;
    if (isNegative) {
        if (absValue == 9223372036854775808ULL) {
            int64Value = INT64_MIN;
        } else {
            int64Value = -static_cast<int64_t>(absValue);
        }
    } else {
        int64Value = static_cast<int64_t>(absValue);
    }
   
    return ConstantInt::get(context.getContext(), APInt(64, int64Value, true));
}
llvm::Value* ExpressionCodeGen::codegenVariable(CodeGen& context, VariableExpr& expr) {
    auto& namedValues = context.getNamedValues();
    auto var = namedValues[expr.getName()];
    if (!var) {
        throw std::runtime_error("Unknown variable: " + expr.getName());
    }
    
    auto& variableTypes = context.getVariableTypes();
    auto typeIt = variableTypes.find(expr.getName());
    
    if (typeIt != variableTypes.end() && typeIt->second == AST::VarType::STRING) {
        auto& builder = context.getBuilder();
        return builder.CreateLoad(llvm::PointerType::get(Type::getInt8Ty(context.getContext()), 0), var, expr.getName().c_str());
    } else {
        auto& builder = context.getBuilder();
        llvm::Type* loadType = Type::getInt64Ty(context.getContext());
        if (typeIt != variableTypes.end()) {
            loadType = context.getLLVMType(typeIt->second);
        }
        return builder.CreateLoad(loadType, var, expr.getName().c_str());
    }
}

llvm::Value* ExpressionCodeGen::codegenBinary(CodeGen& context, BinaryExpr& expr) {
    auto lhs = expr.getLHS()->codegen(context);
    auto rhs = expr.getRHS()->codegen(context);
    auto& builder = context.getBuilder();
    
    if (lhs->getType()->isPointerTy() && rhs->getType()->isPointerTy()) {
        if (expr.getOp() == BinaryOp::ADD) {
            return lhs;
        } else {
            throw std::runtime_error("Unsupported operation for strings");
        }
    }
   
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
        case BinaryOp::DIVIDE: return builder.CreateSDiv(lhs, rhs, "divtmp");
        default: throw std::runtime_error("Unknown binary operator");
    }
}
llvm::Value* ExpressionCodeGen::codegenCall(CodeGen& context, CallExpr& expr) {
    auto& module = context.getModule();
    auto& builder = context.getBuilder();
    
    if (expr.getCallee() == "@println") {
        if (expr.getArgs().size() != 1) {
            throw std::runtime_error("println expects exactly one argument");
        }
        
        auto argValue = expr.getArgs()[0]->codegen(context);
        auto printfFunc = module.getFunction("printf");
        
        if (!printfFunc) {
            throw std::runtime_error("printf function not found");
        }
        
        if (argValue->getType()->isPointerTy()) {
            auto formatStr = builder.CreateGlobalStringPtr("%s\n");
            std::vector<Value*> args = {formatStr, argValue};
            return builder.CreateCall(printfFunc, args);
        } else {
            auto formatStr = builder.CreateGlobalStringPtr("%ld\n");
            std::vector<Value*> args = {formatStr, argValue};
            return builder.CreateCall(printfFunc, args);
        }
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