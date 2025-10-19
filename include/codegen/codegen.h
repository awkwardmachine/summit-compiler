#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <map>
#include <iostream>
#include <unordered_map>

#include "ast/ast_types.h"

namespace llvm {
    class StructType;
}

namespace AST {
    class Expr;
}

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
    class BreakStmt;
    class ContinueStmt;
    class Program;
    class StructDecl;
    class StructLiteralExpr;
    class MemberAssignmentStmt;
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
    
    std::unordered_map<std::string, llvm::StructType*> structTypes;
    std::unordered_map<std::string, std::unordered_map<std::string, int>> structFieldIndices;
    std::map<std::string, std::string> variableStructNames;
    std::unordered_set<std::string> globalVariables;
    
    std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<AST::Expr>>> structFieldDefaults_;
    std::unordered_map<std::string, std::unordered_map<std::string, llvm::Constant*>> structFieldDefaults;
    std::unordered_map<std::string, std::vector<std::pair<std::string, AST::VarType>>> structFields_;
public:
    CodeGen();
    
    /* Get references to LLVM core objects */
    llvm::LLVMContext& getContext() { return *llvmContext; }
    llvm::IRBuilder<>& getBuilder() { return *irBuilder; }
    llvm::Module& getModule() { return *llvmModule; }

    void registerGlobalVariable(const std::string& name) {
        globalVariables.insert(name);
    }
    
    void setGlobalVariables(const std::unordered_set<std::string>& globals) {
        globalVariables = globals;
        std::cout << "DEBUG: Set " << globalVariables.size() << " global variables in codegen" << std::endl;
        for (const auto& global : globalVariables) {
            std::cout << "DEBUG: Global variable: " << global << std::endl;
        }
    }
    
    bool isGlobalVariable(const std::string& name) const {
        return globalVariables.count(name) > 0;
    }
    
    const std::unordered_map<std::string, llvm::StructType*>& getStructTypes() const {
        return structTypes;
    }

    /* Struct type management */
    void registerStructType(const std::string& name, llvm::StructType* type, 
                           const std::vector<std::pair<std::string, AST::VarType>>& fields);
    int getStructFieldIndex(const std::string& structName, const std::string& fieldName);
    llvm::Type* getStructType(const std::string& name);

    const std::vector<std::pair<std::string, AST::VarType>>& getStructFields(const std::string& structName) const;
    
    /* Type conversion */
    llvm::Type* getLLVMType(AST::VarType type, const std::string& structName = "");

    void setVariableStructName(const std::string& varName, const std::string& structName) {
        variableStructNames[varName] = structName;
        std::cout << "DEBUG: Set variable '" << varName << "' to struct '" << structName << "'" << std::endl;
    }

    std::string getVariableStructName(const std::string& varName) const {
        auto it = variableStructNames.find(varName);
        if (it != variableStructNames.end()) {
            return it->second;
        }
        return "";
    }
    
    
    /* Scope management methods */
    void enterScope();
    void exitScope();
    std::unordered_map<std::string, llvm::Value*>& getNamedValues();
    std::unordered_map<std::string, AST::VarType>& getVariableTypes();
    std::unordered_set<std::string>& getConstVariables();

    void setCurrentTargetType(const std::string& type) { currentTargetType = type; }
    std::string getCurrentTargetType() const { return currentTargetType; }
    void clearCurrentTargetType() { currentTargetType.clear(); }

    /* Cross-scope variable lookup */
    llvm::Value* lookupVariable(const std::string& name);
    AST::VarType lookupVariableType(const std::string& name);
    bool isVariableConst(const std::string& name);
    
    /* Type conversion and utility methods */
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
    llvm::Value* codegen(AST::StructLiteralExpr& expr);
    
    /* Statement code generation methods */
    llvm::Value* codegen(AST::VariableDecl& decl);
    llvm::Value* codegen(AST::AssignmentStmt& stmt);
    llvm::Value* codegen(AST::MemberAssignmentStmt& stmt);
    llvm::Value* codegen(AST::ExprStmt& stmt);
    llvm::Value* codegen(AST::IfStmt& stmt);
    llvm::Value* codegen(AST::BlockStmt& stmt);
    llvm::Value* codegen(AST::FunctionStmt& stmt);
    llvm::Value* codegen(AST::ReturnStmt& stmt);
    llvm::Value* codegen(AST::WhileStmt& stmt);
    llvm::Value* codegen(AST::ForLoopStmt& stmt);
    llvm::Value* codegen(AST::EnumDecl& expr);
    llvm::Value* codegen(AST::BreakStmt& expr);
    llvm::Value* codegen(AST::ContinueStmt& expr);
    llvm::Value* codegen(AST::StructDecl& expr);
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

    void pushLoopBlocks(llvm::BasicBlock* exitBlock, llvm::BasicBlock* continueBlock) {
        loopExitBlocks.push_back(exitBlock);
        loopContinueBlocks.push_back(continueBlock);
    }
    
    void popLoopBlocks() {
        if (!loopExitBlocks.empty()) {
            loopExitBlocks.pop_back();
            loopContinueBlocks.pop_back();
        }
    }
    
    llvm::BasicBlock* getCurrentLoopExitBlock() const {
        if (loopExitBlocks.empty()) return nullptr;
        return loopExitBlocks.back();
    }
    
    llvm::BasicBlock* getCurrentLoopContinueBlock() const {
        if (loopContinueBlocks.empty()) return nullptr;
        return loopContinueBlocks.back();
    }

    void registerStructFieldDefault(const std::string& structName, const std::string& fieldName, llvm::Constant* defaultValue) {
        structFieldDefaults[structName][fieldName] = defaultValue;
    }
    
    bool hasStructFieldDefault(const std::string& structName, const std::string& fieldName) const {
        auto structIt = structFieldDefaults.find(structName);
        if (structIt == structFieldDefaults.end()) return false;
        return structIt->second.find(fieldName) != structIt->second.end();
    }
    
    llvm::Constant* getStructFieldDefault(const std::string& structName, const std::string& fieldName) const {
        auto structIt = structFieldDefaults.find(structName);
        if (structIt == structFieldDefaults.end()) return nullptr;
        auto fieldIt = structIt->second.find(fieldName);
        if (fieldIt == structIt->second.end()) return nullptr;
        return fieldIt->second;
    }
private:
    std::string currentTargetType;
    std::vector<llvm::BasicBlock*> loopExitBlocks;
    std::vector<llvm::BasicBlock*> loopContinueBlocks;
};