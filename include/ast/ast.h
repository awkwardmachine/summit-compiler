#pragma once
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <llvm/IR/Value.h>
#include "utils/bigint.h"
#include "ast/ast_types.h"
#include "codegen/bounds.h"

class CodeGen;

namespace AST {

inline std::string indentStr(int indent) {
    return std::string(indent * 2, ' ');
}

inline std::string quoted(const std::string& s) {
    return "\"" + s + "\"";
}

inline std::string boolStr(bool b) { return b ? "true" : "false"; }

class Expr {
public:
    virtual ~Expr() = default;
    virtual llvm::Value* codegen(::CodeGen& context) = 0;
    virtual std::string toString(int indent = 0) const = 0;
};

class Stmt {
public:
    virtual ~Stmt() = default;
    virtual llvm::Value* codegen(::CodeGen& context) = 0;
    virtual std::string toString(int indent = 0) const = 0;
};

class StringExpr : public Expr {
    std::string value;
public:
    StringExpr(const std::string& val) : value(val) {}
    llvm::Value* codegen(::CodeGen& context) override;
    const std::string& getValue() const { return value; }
    std::string toString(int indent = 0) const override {
        return indentStr(indent) + "StringExpr: " + quoted(value);
    }
};
class NumberExpr : public Expr {
    BigInt value;
public:
    NumberExpr(const BigInt& val) : value(val) {}
    NumberExpr(const std::string& str) {
        if (str.length() >= 2 && str.substr(0, 2) == "0b") {
            std::string binStr = str.substr(2);
            binStr.erase(std::remove(binStr.begin(), binStr.end(), '_'), binStr.end());
            
            if (binStr.empty()) {
                throw std::runtime_error("Invalid binary literal: " + str);
            }
            
            uint64_t decimalValue = 0;
            for (char c : binStr) {
                if (c != '0' && c != '1') {
                    throw std::runtime_error("Invalid binary digit: " + std::string(1, c));
                }
                decimalValue = (decimalValue << 1) | (c - '0');
            }
            
            value = BigInt(std::to_string(decimalValue));
        }
        else if (str.length() >= 2 && str.substr(0, 2) == "0x") {
            std::string hexStr = str.substr(2);
            hexStr.erase(std::remove(hexStr.begin(), hexStr.end(), '_'), hexStr.end());
            
            if (hexStr.empty()) {
                throw std::runtime_error("Invalid hex literal: " + str);
            }
            
            uint64_t decimalValue = 0;
            for (char c : hexStr) {
                int digit;
                if (c >= '0' && c <= '9') digit = c - '0';
                else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
                else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
                else throw std::runtime_error("Invalid hex digit: " + std::string(1, c));
                
                decimalValue = (decimalValue << 4) | digit;
            }
            
            value = BigInt(std::to_string(decimalValue));
        }
        else {
            std::string decStr = str;
            decStr.erase(std::remove(decStr.begin(), decStr.end(), '_'), decStr.end());
            value = BigInt(decStr);
        }
    }
    
    llvm::Value* codegen(::CodeGen& context) override;
    const BigInt& getValue() const { return value; }
    std::string toString(int indent = 0) const override {
        return indentStr(indent) + "NumberExpr: " + value.toString();
    }
};

class FormatStringExpr : public Expr {
    std::string formatStr;
    std::vector<std::unique_ptr<Expr>> expressions;
public:
    FormatStringExpr(std::string formatStr, std::vector<std::unique_ptr<Expr>> exprs)
        : formatStr(std::move(formatStr)), expressions(std::move(exprs)) {}

    llvm::Value* codegen(::CodeGen& context) override;
    const std::string& getFormatStr() const { return formatStr; }
    const std::vector<std::unique_ptr<Expr>>& getExpressions() const { return expressions; }
    
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "FormatStringExpr: " << quoted(formatStr) 
            << " with " << expressions.size() << " expression(s)\n";
        for (const auto& expr : expressions) {
            oss << expr->toString(indent + 1) << "\n";
        }
        return oss.str();
    }
};

class FloatExpr : public Expr {
    double value;
    VarType floatType;
public:
    FloatExpr(double val, VarType type = VarType::FLOAT64) : value(val), floatType(type) {}
    double getValue() const { return value; }
    VarType getFloatType() const { return floatType; }
    llvm::Value* codegen(CodeGen& context) override;
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) 
            << "FloatExpr: " << std::fixed << std::setprecision(6) << value
            << " [" << static_cast<int>(floatType) << "]";
        return oss.str();
    }
};

class BooleanExpr : public Expr {
    bool value;
public:
    BooleanExpr(bool val) : value(val) {}
    llvm::Value* codegen(::CodeGen& context) override;
    bool getValue() const { return value; }
    std::string toString(int indent = 0) const override {
        return indentStr(indent) + "BooleanExpr: " + boolStr(value);
    }
};

class VariableExpr : public Expr {
    std::string name;
public:
    VariableExpr(const std::string& name) : name(name) {}
    llvm::Value* codegen(::CodeGen& context) override;
    const std::string& getName() const { return name; }
    std::string toString(int indent = 0) const override {
        return indentStr(indent) + "VariableExpr: " + quoted(name);
    }
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
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "BinaryExpr (Op: " << static_cast<int>(op) << ")\n";
        oss << (lhs ? lhs->toString(indent + 1) : indentStr(indent + 1) + "null") << "\n";
        oss << (rhs ? rhs->toString(indent + 1) : indentStr(indent + 1) + "null");
        return oss.str();
    }
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
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "CallExpr: " << quoted(callee) << " with " << args.size() << " arg(s)\n";
        for (const auto& arg : args)
            oss << arg->toString(indent + 1) << "\n";
        return oss.str();
    }
};

class CastExpr : public Expr {
    std::unique_ptr<Expr> expr;
    VarType targetType;
public:
    CastExpr(std::unique_ptr<Expr> expr, VarType targetType)
        : expr(std::move(expr)), targetType(targetType) {}
    llvm::Value* codegen(::CodeGen& context) override;
    Expr* getExpr() const { return expr.get(); }
    VarType getTargetType() const { return targetType; }
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "CastExpr -> " << static_cast<int>(targetType) << "\n";
        oss << (expr ? expr->toString(indent + 1) : indentStr(indent + 1) + "null");
        return oss.str();
    }
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
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "VariableDecl: " << quoted(name) 
            << " Type: " << static_cast<int>(type)
            << " " << (isConst ? "(const)" : "(var)") << "\n";
        oss << (value ? value->toString(indent + 1) : indentStr(indent + 1) + "null");
        return oss.str();
    }
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
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "AssignmentStmt: " << quoted(name) << "\n";
        oss << (value ? value->toString(indent + 1) : indentStr(indent + 1) + "null");
        return oss.str();
    }
};

class BlockStmt : public Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;
public:
    void addStatement(std::unique_ptr<Stmt> stmt) {
        statements.push_back(std::move(stmt));
    }
    
    llvm::Value* codegen(::CodeGen& context) override;
    
    const std::vector<std::unique_ptr<Stmt>>& getStatements() const { return statements; }
    
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "BlockStmt (" << statements.size() << " stmt(s))\n";
        for (const auto& stmt : statements)
            oss << stmt->toString(indent + 1) << "\n";
        return oss.str();
    }
};

class IfStmt : public Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;
public:
    IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> thenBranch, std::unique_ptr<Stmt> elseBranch = nullptr)
        : condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}
    
    llvm::Value* codegen(::CodeGen& context) override;
    
    const std::unique_ptr<Expr>& getCondition() const { return condition; }
    const std::unique_ptr<Stmt>& getThenBranch() const { return thenBranch; }
    const std::unique_ptr<Stmt>& getElseBranch() const { return elseBranch; }
    
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "IfStmt\n";
        oss << indentStr(indent + 1) << "Condition:\n";
        oss << (condition ? condition->toString(indent + 2) : indentStr(indent + 2) + "null") << "\n";
        oss << indentStr(indent + 1) << "Then:\n";
        oss << (thenBranch ? thenBranch->toString(indent + 2) : indentStr(indent + 2) + "null") << "\n";
        if (elseBranch) {
            oss << indentStr(indent + 1) << "Else:\n";
            oss << elseBranch->toString(indent + 2);
        }
        return oss.str();
    }
};

class FunctionStmt : public Stmt {
    std::string name;
    std::vector<std::pair<std::string, VarType>> parameters;
    VarType returnType;
    std::unique_ptr<BlockStmt> body;
public:
    FunctionStmt(const std::string& name, 
                 std::vector<std::pair<std::string, VarType>> parameters,
                 VarType returnType,
                 std::unique_ptr<BlockStmt> body)
        : name(name), parameters(std::move(parameters)), 
          returnType(returnType), body(std::move(body)) {}
    
    llvm::Value* codegen(::CodeGen& context) override;
    
    const std::string& getName() const { return name; }
    const std::vector<std::pair<std::string, VarType>>& getParameters() const { return parameters; }
    VarType getReturnType() const { return returnType; }
    const std::unique_ptr<BlockStmt>& getBody() const { return body; }
    
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "FunctionStmt: " << quoted(name) 
            << " -> " << static_cast<int>(returnType) << "\n";
        oss << indentStr(indent + 1) << "Parameters (" << parameters.size() << "):\n";
        for (const auto& param : parameters) {
            oss << indentStr(indent + 2) << quoted(param.first) 
                << " : " << static_cast<int>(param.second) << "\n";
        }
        oss << indentStr(indent + 1) << "Body:\n";
        oss << (body ? body->toString(indent + 2) : indentStr(indent + 2) + "null");
        return oss.str();
    }
};

class ReturnStmt : public Stmt {
    std::unique_ptr<Expr> value;
public:
    ReturnStmt(std::unique_ptr<Expr> value = nullptr) : value(std::move(value)) {}
    
    llvm::Value* codegen(::CodeGen& context) override;
    
    const std::unique_ptr<Expr>& getValue() const { return value; }
    
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "ReturnStmt\n";
        oss << (value ? value->toString(indent + 1) : indentStr(indent + 1) + "void");
        return oss.str();
    }
};

class UnaryExpr : public Expr {
    UnaryOp op;
    std::unique_ptr<Expr> operand;
public:
    UnaryExpr(UnaryOp op, std::unique_ptr<Expr> operand)
        : op(op), operand(std::move(operand)) {}
    
    llvm::Value* codegen(CodeGen& context) override;
    UnaryOp getOp() const { return op; }
    Expr* getOperand() const { return operand.get(); }
    
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "UnaryExpr (Op: " << static_cast<int>(op) << ")\n";
        oss << (operand ? operand->toString(indent + 1) : indentStr(indent + 1) + "null");
        return oss.str();
    }
};

class ExprStmt : public Stmt {
    std::unique_ptr<Expr> expr;
public:
    ExprStmt(std::unique_ptr<Expr> expr) : expr(std::move(expr)) {}
    llvm::Value* codegen(::CodeGen& context) override;
    const std::unique_ptr<Expr>& getExpr() const { return expr; }
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "ExprStmt\n";
        oss << (expr ? expr->toString(indent + 1) : indentStr(indent + 1) + "null");
        return oss.str();
    }
};

class Program {
    std::vector<std::unique_ptr<Stmt>> statements;
public:
    void addStatement(std::unique_ptr<Stmt> stmt) {
        statements.push_back(std::move(stmt));
    }
    llvm::Value* codegen(::CodeGen& context);
    const std::vector<std::unique_ptr<Stmt>>& getStatements() const { return statements; }
    std::string toString(int indent = 0) const {
        std::ostringstream oss;
        oss << indentStr(indent) << "Program (" << statements.size() << " stmt(s))\n";
        for (const auto& stmt : statements)
            oss << stmt->toString(indent + 1) << "\n";
        return oss.str();
    }
};

}
