#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <map>
#include <iostream>
#include <unordered_map>

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
    class FunctionStmt;
    class ReturnStmt;
    class WhileStmt;
    class ForLoopStmt;
    class ModuleExpr;
    class MemberAccessExpr;
    class EnumDecl;
    class EnumValueExpr;
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
    std::map<std::string, llvm::Value*> moduleReferences;
    std::map<std::string, std::string> moduleIdentities;
    std::map<std::string, std::string> moduleAliases;
    std::unordered_map<std::string, llvm::Value*> moduleReferencesMap;
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
    llvm::Value* codegen(AST::ModuleExpr& expr);
    llvm::Value* codegen(AST::MemberAccessExpr& expr);
    llvm::Value* codegen(AST::EnumValueExpr& expr);
    
    /* Statement code generation methods */
    llvm::Value* codegen(AST::VariableDecl& decl);
    llvm::Value* codegen(AST::AssignmentStmt& stmt);
    llvm::Value* codegen(AST::ExprStmt& stmt);
    llvm::Value* codegen(AST::IfStmt& stmt);
    llvm::Value* codegen(AST::BlockStmt& stmt);
    llvm::Value* codegen(AST::FunctionStmt& stmt);
    llvm::Value* codegen(AST::ReturnStmt& stmt);
    llvm::Value* codegen(AST::WhileStmt& stmt);
    llvm::Value* codegen(AST::ForLoopStmt& stmt);
    llvm::Value* codegen(AST::EnumDecl& expr);
    llvm::Value* codegen(AST::Program& program);
    
    /* Debugging and output methods */
    void printIR();
    void printIRToFile(const std::string& filename);
    bool compileToExecutable(const std::string& outputFilename, bool verbose = false, 
                        const std::string& targetTriple = "", bool noStdlib = false);

    /* Module alias management */
    void setModuleReference(const std::string& varName, llvm::Value* module, const std::string& actualModuleName) {
        moduleReferences[varName] = module;
        moduleIdentities[varName] = actualModuleName;
        std::cout << "DEBUG: Tracked module alias: " << varName << " -> " << actualModuleName << std::endl;
    }

    void registerModuleAlias(const std::string& alias, const std::string& actualModuleName, llvm::Value* moduleValue);
    std::string resolveModuleAlias(const std::string& name) const;
    
    llvm::Value* getModuleReference(const std::string& varName) const {
        auto it = moduleReferences.find(varName);
        if (it != moduleReferences.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    std::string getModuleIdentity(const std::string& varName) const {
        auto it = moduleIdentities.find(varName);
        if (it != moduleIdentities.end()) {
            return it->second;
        }
        return "";
    }
    
    void clearModuleReferences() {
        moduleReferences.clear();
        moduleIdentities.clear();
    }
};