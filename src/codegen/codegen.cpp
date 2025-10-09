#include "codegen.h"
#include "builtins.h"
#include "expr_codegen.h"
#include "stmt_codegen.h"
#include <llvm/IR/Verifier.h>
#include "codegen/bounds.h"
#include "ast/ast.h"

using namespace llvm;
using namespace AST;

CodeGen::CodeGen() {
    llvmContext = std::make_unique<LLVMContext>();
    irBuilder = std::make_unique<IRBuilder<>>(*llvmContext);
    llvmModule = std::make_unique<Module>("summit", *llvmContext);
   
    namedValues = std::unordered_map<std::string, llvm::Value*>();
    variableTypes = std::unordered_map<std::string, AST::VarType>();
    constVariables = std::unordered_set<std::string>();
    
    createPrintlnFunction();
}

llvm::Type* CodeGen::getLLVMType(AST::VarType type) {
    auto& context = getContext();
    switch (type) {
        case AST::VarType::BOOL: return Type::getInt1Ty(context);
        case AST::VarType::INT4: return Type::getInt8Ty(context);
        case AST::VarType::INT8: return Type::getInt8Ty(context);
        case AST::VarType::INT12: return Type::getInt16Ty(context);
        case AST::VarType::INT16: return Type::getInt16Ty(context);
        case AST::VarType::INT24: return Type::getInt32Ty(context);
        case AST::VarType::INT32: return Type::getInt32Ty(context);
        case AST::VarType::INT48: return Type::getInt64Ty(context);
        case AST::VarType::INT64: return Type::getInt64Ty(context);
        case AST::VarType::UINT4: return Type::getInt8Ty(context);
        case AST::VarType::UINT8: return Type::getInt8Ty(context);
        case AST::VarType::UINT12: return Type::getInt16Ty(context);
        case AST::VarType::UINT16: return Type::getInt16Ty(context);
        case AST::VarType::UINT24: return Type::getInt32Ty(context);
        case AST::VarType::UINT32: return Type::getInt32Ty(context);
        case AST::VarType::UINT48: return Type::getInt64Ty(context);
        case AST::VarType::UINT64: return Type::getInt64Ty(context);
        case AST::VarType::UINT0: return Type::getInt1Ty(context);
        case AST::VarType::FLOAT32: return Type::getFloatTy(context);
        case AST::VarType::FLOAT64: return Type::getDoubleTy(context);
        case AST::VarType::STRING: return PointerType::get(Type::getInt8Ty(context), 0);
        case AST::VarType::VOID: return Type::getVoidTy(context);
        default: throw std::runtime_error("Unknown type");
    }
}

bool CodeGen::isConstVariable(const std::string& name) {
    return constVariables.find(name) != constVariables.end();
}

void CodeGen::createPrintlnFunction() {
    Builtins::createPrintfFunction(*this);
    Builtins::createPrintlnFunction(*this);
}

llvm::Value* CodeGen::codegen(StringExpr& expr) {
    return ExpressionCodeGen::codegenString(*this, expr);
}

llvm::Value* CodeGen::codegen(NumberExpr& expr) {
    return ExpressionCodeGen::codegenNumber(*this, expr);
}

llvm::Value* CodeGen::codegen(FloatExpr& expr) {
    return ExpressionCodeGen::codegenFloat(*this, expr);
}

llvm::Value* CodeGen::codegen(BooleanExpr& expr) {
    return ExpressionCodeGen::codegenBoolean(*this, expr);
}

llvm::Value* CodeGen::codegen(VariableExpr& expr) {
    return ExpressionCodeGen::codegenVariable(*this, expr);
}

llvm::Value* CodeGen::codegen(BinaryExpr& expr) {
    return ExpressionCodeGen::codegenBinary(*this, expr);
}

llvm::Value* CodeGen::codegen(CallExpr& expr) {
    return ExpressionCodeGen::codegenCall(*this, expr);
}

llvm::Value* CodeGen::codegen(VariableDecl& decl) {
    return StatementCodeGen::codegenVariableDecl(*this, decl);
}

llvm::Value* CodeGen::codegen(AssignmentStmt& stmt) {
    return StatementCodeGen::codegenAssignment(*this, stmt);
}

llvm::Value* CodeGen::codegen(ExprStmt& stmt) {
    return StatementCodeGen::codegenExprStmt(*this, stmt);
}

llvm::Value* CodeGen::codegen(Program& program) {
    return StatementCodeGen::codegenProgram(*this, program);
}

void CodeGen::printIR() {
    llvmModule->print(outs(), nullptr);
}

void CodeGen::printIRToFile(const std::string& filename) {
    std::error_code EC;
    llvm::raw_fd_ostream out(filename, EC);
    if (EC) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    llvmModule->print(out, nullptr);
}