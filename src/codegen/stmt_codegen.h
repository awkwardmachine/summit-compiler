#pragma once
#include "codegen.h"
#include "utils/bigint.h"

namespace StatementCodeGen {
    llvm::Value* codegenVariableDecl(CodeGen& context, AST::VariableDecl& decl);
    llvm::Value* codegenAssignment(CodeGen& context, AST::AssignmentStmt& stmt);
    llvm::Value* codegenExprStmt(CodeGen& context, AST::ExprStmt& stmt);
    llvm::Value* codegenProgram(CodeGen& context, AST::Program& program);
}