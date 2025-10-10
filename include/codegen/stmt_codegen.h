#pragma once
#include "codegen.h"
#include "utils/bigint.h"

namespace StatementCodeGen {
    llvm::Value* codegenVariableDecl(CodeGen& context, AST::VariableDecl& decl);
    llvm::Value* codegenAssignment(CodeGen& context, AST::AssignmentStmt& stmt);
    llvm::Value* codegenExprStmt(CodeGen& context, AST::ExprStmt& stmt);
    llvm::Value* codegenIfStmt(CodeGen& context, AST::IfStmt& stmt);
    llvm::Value* codegenProgram(CodeGen& context, AST::Program& program);
    llvm::Value* codegenBlockStmt(CodeGen& context, AST::BlockStmt& stmt);
    llvm::Value* codegenFunctionStmt(CodeGen& context, AST::FunctionStmt& stmt);
    llvm::Value* codegenReturnStmt(CodeGen& context, AST::ReturnStmt& stmt);
}