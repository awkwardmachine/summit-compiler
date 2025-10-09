#pragma once
#include <memory>
#include <string>
#include <vector>
#include <llvm/IR/Value.h>
#include "utils/bigint.h"
#include "ast/ast_types.h"

class CodeGen;

namespace AST {

class Expr {
public:
    virtual ~Expr() = default;
    virtual llvm::Value* codegen(::CodeGen& context) = 0;
};

class Stmt {
public:
    virtual ~Stmt() = default;
    virtual llvm::Value* codegen(::CodeGen& context) = 0;
};

class StringExpr : public Expr {
    std::string value;
public:
    StringExpr(const std::string& val) : value(val) {}
    llvm::Value* codegen(::CodeGen& context) override;
    const std::string& getValue() const { return value; }
};

class NumberExpr : public Expr {
    BigInt value;
public:
    NumberExpr(const BigInt& val) : value(val) {}
    NumberExpr(const std::string& str) : value(str) {}
    llvm::Value* codegen(::CodeGen& context) override;
    const BigInt& getValue() const { return value; }
};

class VariableExpr : public Expr {
    std::string name;
public:
    VariableExpr(const std::string& name) : name(name) {}
    llvm::Value* codegen(::CodeGen& context) override;
    const std::string& getName() const { return name; }
};

class BinaryExpr : public Expr {
    BinaryOp op;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
public:
    BinaryExpr(BinaryOp op, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs)
        : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    llvm::Value* codegen(::CodeGen& context) override;
    BinaryOp getOp() const { return op; }
    const std::unique_ptr<Expr>& getLHS() const { return lhs; }
    const std::unique_ptr<Expr>& getRHS() const { return rhs; }
};

class CallExpr : public Expr {
    std::string callee;
    std::vector<std::unique_ptr<Expr>> args;
public:
    CallExpr(const std::string& callee, std::vector<std::unique_ptr<Expr>> args)
        : callee(callee), args(std::move(args)) {}
    llvm::Value* codegen(::CodeGen& context) override;
    const std::string& getCallee() const { return callee; }
    const std::vector<std::unique_ptr<Expr>>& getArgs() const { return args; }
};

class VariableDecl : public Stmt {
    std::string name;
    VarType type;
    bool isConst;
    std::unique_ptr<Expr> value;
public:
    VariableDecl(const std::string& name, VarType type, bool isConst, std::unique_ptr<Expr> value)
        : name(name), type(type), isConst(isConst), value(std::move(value)) {}
    llvm::Value* codegen(::CodeGen& context) override;
    const std::string& getName() const { return name; }
    VarType getType() const { return type; }
    bool getIsConst() const { return isConst; }
    const std::unique_ptr<Expr>& getValue() const { return value; }
};

class AssignmentStmt : public Stmt {
    std::string name;
    std::unique_ptr<Expr> value;
public:
    AssignmentStmt(const std::string& name, std::unique_ptr<Expr> value)
        : name(name), value(std::move(value)) {}
    llvm::Value* codegen(::CodeGen& context) override;
    const std::string& getName() const { return name; }
    const std::unique_ptr<Expr>& getValue() const { return value; }
};

class ExprStmt : public Stmt {
    std::unique_ptr<Expr> expr;
public:
    ExprStmt(std::unique_ptr<Expr> expr) : expr(std::move(expr)) {}
    llvm::Value* codegen(::CodeGen& context) override;
    const std::unique_ptr<Expr>& getExpr() const { return expr; }
};

class Program {
    std::vector<std::unique_ptr<Stmt>> statements;
public:
    void addStatement(std::unique_ptr<Stmt> stmt) {
        statements.push_back(std::move(stmt));
    }
    llvm::Value* codegen(::CodeGen& context);
    const std::vector<std::unique_ptr<Stmt>>& getStatements() const { return statements; }
};

}