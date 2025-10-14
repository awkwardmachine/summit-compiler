#pragma once
#include "codegen.h"
#include "utils/bigint.h"
#include "bounds.h"
#include "ast.h"

namespace StatementCodeGen {
    llvm::Value* codegenVariableDecl(CodeGen& context, AST::VariableDecl& decl);
    llvm::Value* codegenGlobalVariable(CodeGen& context, AST::VariableDecl& decl);
    llvm::Value* codegenAssignment(CodeGen& context, AST::AssignmentStmt& stmt);
    llvm::Value* codegenExprStmt(CodeGen& context, AST::ExprStmt& stmt);
    llvm::Value* codegenIfStmt(CodeGen& context, AST::IfStmt& stmt);
    llvm::Value* codegenProgram(CodeGen& context, AST::Program& program);
    llvm::Value* codegenBlockStmt(CodeGen& context, AST::BlockStmt& stmt);
    llvm::Value* codegenFunctionStmt(CodeGen& context, AST::FunctionStmt& stmt);
    llvm::Value* codegenReturnStmt(CodeGen& context, AST::ReturnStmt& stmt);
    llvm::Value* codegenWhileStmt(CodeGen& context, AST::WhileStmt& stmt);
    llvm::Value* codegenForLoopStmt(CodeGen& context, AST::ForLoopStmt& stmt);
    llvm::Value* codegenEnumDecl(CodeGen& context, AST::EnumDecl& stmt);
    llvm::Value* codegenContinueStmt(CodeGen& context, AST::ContinueStmt& stmt);
    llvm::Value* codegenBreakStmt(CodeGen& context, AST::BreakStmt& stmt);

    llvm::Value* addRuntimeBoundsChecking(CodeGen& context, llvm::Value* value, AST::VarType targetType, const std::string& varName);
}