#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include "ast/ast_types.h"

namespace AST {
    enum class VarType;
    class NumberExpr;
    class StringExpr;
    class VariableExpr;
    class BinaryExpr;
    class CallExpr;
    class VariableDecl;
    class AssignmentStmt;
    class ExprStmt;
    class Program;
}

class CodeGen {
    std::unique_ptr<llvm::LLVMContext> llvmContext;
    std::unique_ptr<llvm::IRBuilder<>> irBuilder;
    std::unique_ptr<llvm::Module> llvmModule;
    
    std::unordered_map<std::string, llvm::Value*> namedValues;
    std::unordered_map<std::string, AST::VarType> variableTypes;
    std::unordered_set<std::string> constVariables;

public:
    CodeGen();
    
    llvm::LLVMContext& getContext() { return *llvmContext; }
    llvm::IRBuilder<>& getBuilder() { return *irBuilder; }
    llvm::Module& getModule() { return *llvmModule; }
    std::unordered_map<std::string, llvm::Value*>& getNamedValues() { return namedValues; }
    std::unordered_map<std::string, AST::VarType>& getVariableTypes() { return variableTypes; }
    std::unordered_set<std::string>& getConstVariables() { return constVariables; }
    
    llvm::Type* getLLVMType(AST::VarType type);
    bool isConstVariable(const std::string& name);
    
    void createPrintlnFunction();
    
    llvm::Value* codegen(AST::NumberExpr& expr);
    llvm::Value* codegen(AST::StringExpr& expr);
    llvm::Value* codegen(AST::VariableExpr& expr);
    llvm::Value* codegen(AST::BinaryExpr& expr);
    llvm::Value* codegen(AST::CallExpr& expr);
    llvm::Value* codegen(AST::VariableDecl& decl);
    llvm::Value* codegen(AST::AssignmentStmt& stmt);
    llvm::Value* codegen(AST::ExprStmt& stmt);
    llvm::Value* codegen(AST::Program& program);
    
    void printIR();
    void printIRToFile(const std::string& filename);
};