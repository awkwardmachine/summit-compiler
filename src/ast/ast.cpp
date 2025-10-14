#include "ast/ast.h"
#include "codegen/codegen.h"

using namespace llvm;
using namespace AST;

llvm::Value* StringExpr::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* FormatStringExpr::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* NumberExpr::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* FloatExpr::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* VariableExpr::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* BinaryExpr::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* CallExpr::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* UnaryExpr::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* VariableDecl::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* AssignmentStmt::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* BlockStmt::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* IfStmt::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* ExprStmt::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* BooleanExpr::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* CastExpr::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* Program::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* FunctionStmt::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* ReturnStmt::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* WhileStmt::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* ForLoopStmt::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* ModuleExpr::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* MemberAccessExpr::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* EnumValueExpr::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* EnumDecl::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* ContinueStmt::codegen(::CodeGen& context) {
    return context.codegen(*this);
}

llvm::Value* BreakStmt::codegen(::CodeGen& context) {
    return context.codegen(*this);
}