#pragma once
#include "codegen.h"
#include "ast/ast.h"
#include "utils/bigint.h"

namespace ExpressionCodeGen {
    llvm::Value* codegenNumber(CodeGen& context, AST::NumberExpr& expr);
    llvm::Value* codegenString(CodeGen& context, AST::StringExpr& expr);
    llvm::Value* codegenVariable(CodeGen& context, AST::VariableExpr& expr);
    llvm::Value* codegenBinary(CodeGen& context, AST::BinaryExpr& expr);
    llvm::Value* codegenBoolean(CodeGen& context, AST::BooleanExpr& expr);
    llvm::Value* codegenCall(CodeGen& context, AST::CallExpr& expr);
    llvm::Value* codegenFloat(CodeGen& context, AST::FloatExpr& expr);
    llvm::Value* codegenCast(CodeGen& context, AST::CastExpr& expr);
    llvm::Value* codegenUnary(CodeGen& context, AST::UnaryExpr& expr);
    llvm::Value* codegenFormatString(CodeGen& context, AST::FormatStringExpr& expr);
    llvm::Value* codegenModule(CodeGen& context, AST::ModuleExpr& expr);
    llvm::Value* codegenMemberAccess(CodeGen& context, AST::MemberAccessExpr& expr);
    llvm::Value* codegenEnumValue(CodeGen& context, AST::EnumValueExpr& expr);
    llvm::Value* codegenStructLiteral(CodeGen& context, AST::StructLiteralExpr& expr);
    std::string extractModuleName(CodeGen& context, const std::string& varName);
    llvm::Value* handleModuleMemberAccess(CodeGen& context, const std::string& moduleName, const std::string& member);

    llvm::Value* addSimpleBoundsChecking(CodeGen& context, llvm::Value* value, const std::string& targetType);
}