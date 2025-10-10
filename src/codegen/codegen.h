#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include "ast/ast_types.h"

namespace AST {
    enum class VarType;
    class NumberExpr;
    class UnaryExpr;
    class FloatExpr;
    class StringExpr;
    class FormatStringExpr;
    class VariableExpr;
    class BooleanExpr;
    class BinaryExpr;
    class CallExpr;
    class VariableDecl;
    class CastExpr;
    class AssignmentStmt;
    class ExprStmt;
    class IfStmt;
    class BlockStmt;
    class Program;
}

class CodeGen {
    std::unique_ptr<llvm::LLVMContext> llvmContext;
    std::unique_ptr<llvm::IRBuilder<>> irBuilder;
    std::unique_ptr<llvm::Module> llvmModule;
    
    /* Stack-based scope management for variables */
    std::vector<std::unordered_map<std::string, llvm::Value*>> namedValuesStack;
    std::vector<std::unordered_map<std::string, AST::VarType>> variableTypesStack;
    std::vector<std::unordered_set<std::string>> constVariablesStack;

public:
    CodeGen();
    
    /* Get references to LLVM core objects */
    llvm::LLVMContext& getContext() { return *llvmContext; }
    llvm::IRBuilder<>& getBuilder() { return *irBuilder; }
    llvm::Module& getModule() { return *llvmModule; }
    
    /* Scope management methods */
    void enterScope();
    void exitScope();
    std::unordered_map<std::string, llvm::Value*>& getNamedValues();
    std::unordered_map<std::string, AST::VarType>& getVariableTypes();
    std::unordered_set<std::string>& getConstVariables();

    /* Cross-scope variable lookup */
    llvm::Value* lookupVariable(const std::string& name);
    AST::VarType lookupVariableType(const std::string& name);
    bool isVariableConst(const std::string& name);
    
    /* Type conversion and utility methods */
    llvm::Type* getLLVMType(AST::VarType type);
    bool isConstVariable(const std::string& name) { return isVariableConst(name); }
    
    /* Builtin function creation */
    void createPrintlnFunction();
    
    /* Expression code generation methods */
    llvm::Value* codegen(AST::NumberExpr& expr);
    llvm::Value* codegen(AST::UnaryExpr& expr);
    llvm::Value* codegen(AST::FloatExpr& expr);
    llvm::Value* codegen(AST::StringExpr& expr);
    llvm::Value* codegen(AST::FormatStringExpr& expr);
    llvm::Value* codegen(AST::VariableExpr& expr);
    llvm::Value* codegen(AST::BinaryExpr& expr);
    llvm::Value* codegen(AST::CallExpr& expr);
    llvm::Value* codegen(AST::CastExpr& expr);
    llvm::Value* codegen(AST::BooleanExpr& expr);
    
    /* Statement code generation methods */
    llvm::Value* codegen(AST::VariableDecl& decl);
    llvm::Value* codegen(AST::AssignmentStmt& stmt);
    llvm::Value* codegen(AST::ExprStmt& stmt);
    llvm::Value* codegen(AST::IfStmt& stmt);
    llvm::Value* codegen(AST::BlockStmt& stmt);
    llvm::Value* codegen(AST::Program& program);
    
    /* Debugging and output methods */
    void printIR();
    void printIRToFile(const std::string& filename);
};