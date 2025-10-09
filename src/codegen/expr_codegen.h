#pragma once
#include "codegen.h"
#include "ast/ast.h"
#include "utils/bigint.h"

namespace ExpressionCodeGen {
    llvm::Value* codegenNumber(CodeGen& context, AST::NumberExpr& expr);
    llvm::Value* codegenString(CodeGen& context, AST::StringExpr& expr);
    llvm::Value* codegenVariable(CodeGen& context, AST::VariableExpr& expr);
    llvm::Value* codegenBinary(CodeGen& context, AST::BinaryExpr& expr);
    llvm::Value* codegenCall(CodeGen& context, AST::CallExpr& expr);
}