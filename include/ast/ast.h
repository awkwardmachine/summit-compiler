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
    std::unique_ptr<Expr> calleeExpr;
    std::vector<std::unique_ptr<Expr>> args;
public:
    CallExpr(const std::string& callee, std::vector<std::unique_ptr<Expr>> args)
        : callee(callee), args(std::move(args)) {}
    
    CallExpr(std::unique_ptr<Expr> calleeExpr, std::vector<std::unique_ptr<Expr>> args)
        : calleeExpr(std::move(calleeExpr)), args(std::move(args)) {}
    
    llvm::Value* codegen(::CodeGen& context) override;
    
    const std::string& getCallee() const { return callee; }
    const std::unique_ptr<Expr>& getCalleeExpr() const { return calleeExpr; }
    const std::vector<std::unique_ptr<Expr>>& getArgs() const { return args; }
    
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "CallExpr: ";
        if (calleeExpr) {
            oss << "(member call)";
        } else {
            oss << quoted(callee);
        }
        oss << " with " << args.size() << " arg(s)\n";
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
    std::string structName;

public:
    VariableDecl(const std::string& name, VarType type, bool isConst, 
                 std::unique_ptr<Expr> value, const std::string& structName = "")
        : name(name), type(type), isConst(isConst), 
          value(std::move(value)), structName(structName) {}
    
    llvm::Value* codegen(::CodeGen& context) override;
    
    const std::string& getName() const { return name; }
    VarType getType() const { return type; }
    bool getIsConst() const { return isConst; }
    const std::unique_ptr<Expr>& getValue() const { return value; }
    const std::string& getStructName() const { return structName; }
    
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "VariableDecl: " << quoted(name) 
            << " Type: " << static_cast<int>(type);
        
        if (type == VarType::STRUCT && !structName.empty()) {
            oss << " (" << structName << ")";
        }
        
        oss << " " << (isConst ? "(const)" : "(var)") << "\n";
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
    bool isEntryPoint;
public:
    FunctionStmt(const std::string& name, 
                 std::vector<std::pair<std::string, VarType>> parameters,
                 VarType returnType,
                 std::unique_ptr<BlockStmt> body,
                 bool isEntryPoint = false)
        : name(name), parameters(std::move(parameters)), 
          returnType(returnType), body(std::move(body)), isEntryPoint(isEntryPoint) {};
    
    llvm::Value* codegen(::CodeGen& context) override;
    
    const std::string& getName() const { return name; }
    const std::vector<std::pair<std::string, VarType>>& getParameters() const { return parameters; }
    VarType getReturnType() const { return returnType; }
    const std::unique_ptr<BlockStmt>& getBody() const { return body; }
    bool getIsEntryPoint() const { return isEntryPoint; }
    void setIsEntryPoint(bool value) { isEntryPoint = value; }
    
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "FunctionStmt: " << quoted(name) 
            << " -> " << static_cast<int>(returnType) 
            << (isEntryPoint ? " [ENTRYPOINT]" : "") << "\n";
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

class WhileStmt : public Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> body;
public:
    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<BlockStmt> body)
        : condition(std::move(condition)), body(std::move(body)) {}
    
    llvm::Value* codegen(::CodeGen& context) override;
    
    const std::unique_ptr<Expr>& getCondition() const { return condition; }
    const std::unique_ptr<BlockStmt>& getBody() const { return body; }
    
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "WhileStmt\n";
        oss << indentStr(indent + 1) << "Condition:\n";
        oss << (condition ? condition->toString(indent + 2) : indentStr(indent + 2) + "null") << "\n";
        oss << indentStr(indent + 1) << "Body:\n";
        oss << (body ? body->toString(indent + 2) : indentStr(indent + 2) + "null");
        return oss.str();
    }
};

class ForLoopStmt : public Stmt {
    std::string varName;
    VarType varType;
    std::unique_ptr<Expr> initializer;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> increment;
    std::unique_ptr<BlockStmt> body;
public:
    ForLoopStmt(const std::string& varName, VarType varType,
                std::unique_ptr<Expr> initializer,
                std::unique_ptr<Expr> condition,
                std::unique_ptr<Expr> increment,
                std::unique_ptr<BlockStmt> body)
        : varName(varName), varType(varType), initializer(std::move(initializer)),
          condition(std::move(condition)), increment(std::move(increment)),
          body(std::move(body)) {}
    
    llvm::Value* codegen(::CodeGen& context) override;
    
    const std::string& getVarName() const { return varName; }
    VarType getVarType() const { return varType; }
    const std::unique_ptr<Expr>& getInitializer() const { return initializer; }
    const std::unique_ptr<Expr>& getCondition() const { return condition; }
    const std::unique_ptr<Expr>& getIncrement() const { return increment; }
    const std::unique_ptr<BlockStmt>& getBody() const { return body; }
    
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "ForLoopStmt: " << quoted(varName) 
            << " : " << static_cast<int>(varType) << "\n";
        oss << indentStr(indent + 1) << "Initializer:\n";
        oss << (initializer ? initializer->toString(indent + 2) : indentStr(indent + 2) + "null") << "\n";
        oss << indentStr(indent + 1) << "Condition:\n";
        oss << (condition ? condition->toString(indent + 2) : indentStr(indent + 2) + "null") << "\n";
        oss << indentStr(indent + 1) << "Increment:\n";
        oss << (increment ? increment->toString(indent + 2) : indentStr(indent + 2) + "null") << "\n";
        oss << indentStr(indent + 1) << "Body:\n";
        oss << (body ? body->toString(indent + 2) : indentStr(indent + 2) + "null");
        return oss.str();
    }
};

class EntrypointStmt : public Stmt {
public:
    EntrypointStmt() = default;
    
    llvm::Value* codegen(::CodeGen& context) override {
        return nullptr;
    }
    
    std::string toString(int indent = 0) const override {
        return indentStr(indent) + "EntrypointStmt";
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

class ModuleExpr : public Expr {
    std::string moduleName;
public:
    ModuleExpr(const std::string& name) : moduleName(name) {}
    llvm::Value* codegen(::CodeGen& context) override;
    const std::string& getModuleName() const { return moduleName; }
    std::string toString(int indent = 0) const override {
        return indentStr(indent) + "ModuleExpr: " + quoted(moduleName);
    }
};

class MemberAccessExpr : public Expr {
    std::unique_ptr<Expr> object;
    std::string member;
public:
    MemberAccessExpr(std::unique_ptr<Expr> object, const std::string& member)
        : object(std::move(object)), member(member) {}
    llvm::Value* codegen(::CodeGen& context) override;
    const std::unique_ptr<Expr>& getObject() const { return object; }
    const std::string& getMember() const { return member; }
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "MemberAccessExpr: ." << member << "\n";
        oss << (object ? object->toString(indent + 1) : indentStr(indent + 1) + "null");
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

class EnumDecl : public Stmt {
    std::string name;
    std::vector<std::pair<std::string, std::unique_ptr<Expr>>> members;
public:
    EnumDecl(const std::string& name, 
             std::vector<std::pair<std::string, std::unique_ptr<Expr>>> members)
        : name(name), members(std::move(members)) {}
    
    llvm::Value* codegen(::CodeGen& context) override;
    
    const std::string& getName() const { return name; }
    const std::vector<std::pair<std::string, std::unique_ptr<Expr>>>& getMembers() const { return members; }
    
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "EnumDecl: " << quoted(name) 
            << " with " << members.size() << " member(s)\n";
        for (const auto& member : members) {
            oss << indentStr(indent + 1) << quoted(member.first) << " = ";
            if (member.second) {
                oss << member.second->toString(0);
            } else {
                oss << "auto";
            }
            oss << "\n";
        }
        return oss.str();
    }
};
class EnumValueExpr : public Expr {
    std::string enumName;
    std::string memberName;
public:
    EnumValueExpr(const std::string& enumName, const std::string& memberName)
        : enumName(enumName), memberName(memberName) {}
    
    llvm::Value* codegen(::CodeGen& context) override;
    
    const std::string& getEnumName() const { return enumName; }
    const std::string& getMemberName() const { return memberName; }
    
    std::string toString(int indent = 0) const override {
        return indentStr(indent) + "EnumValueExpr: " + enumName + "." + memberName;
    }
};

class BreakStmt : public Stmt {
public:
    BreakStmt() = default;
    
    llvm::Value* codegen(::CodeGen& context) override;
    
    std::string toString(int indent = 0) const override {
        return indentStr(indent) + "BreakStmt";
    }
};

class ContinueStmt : public Stmt {
public:
    ContinueStmt() = default;
    
    llvm::Value* codegen(::CodeGen& context) override;
    
    std::string toString(int indent = 0) const override {
        return indentStr(indent) + "ContinueStmt";
    }
};

class StructDecl : public Stmt {
    std::string name;
    std::vector<std::pair<std::string, VarType>> fields;
    std::vector<std::unique_ptr<FunctionStmt>> methods;
public:
    StructDecl(const std::string& name, 
               std::vector<std::pair<std::string, VarType>> fields,
               std::vector<std::unique_ptr<FunctionStmt>> methods = {})
        : name(name), fields(std::move(fields)), methods(std::move(methods)) {}
    
    llvm::Value* codegen(::CodeGen& context) override;
    
    const std::string& getName() const { return name; }
    const std::vector<std::pair<std::string, VarType>>& getFields() const { return fields; }
    const std::vector<std::unique_ptr<FunctionStmt>>& getMethods() const { return methods; }
    
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "StructDecl: " << quoted(name) 
            << " with " << fields.size() << " field(s) and " 
            << methods.size() << " method(s)\n";
        
        oss << indentStr(indent + 1) << "Fields:\n";
        for (const auto& field : fields) {
            oss << indentStr(indent + 2) << quoted(field.first) 
                << " : " << static_cast<int>(field.second) << "\n";
        }
        
        oss << indentStr(indent + 1) << "Methods:\n";
        for (const auto& method : methods) {
            oss << method->toString(indent + 2) << "\n";
        }
        return oss.str();
    }
};

class StructLiteralExpr : public Expr {
    std::string structName;
    std::vector<std::pair<std::string, std::unique_ptr<Expr>>> fields;
public:
    StructLiteralExpr(const std::string& structName,
                     std::vector<std::pair<std::string, std::unique_ptr<Expr>>> fields)
        : structName(structName), fields(std::move(fields)) {}
    
    llvm::Value* codegen(::CodeGen& context) override;
    
    const std::string& getStructName() const { return structName; }
    const std::vector<std::pair<std::string, std::unique_ptr<Expr>>>& getFields() const { return fields; }
    
    std::string toString(int indent = 0) const override {
        std::ostringstream oss;
        oss << indentStr(indent) << "StructLiteralExpr: " << quoted(structName) 
            << " with " << fields.size() << " field(s)\n";
        for (const auto& field : fields) {
            oss << indentStr(indent + 1) << quoted(field.first) << " = ";
            if (field.second) {
                oss << field.second->toString(0);
            } else {
                oss << "null";
            }
            oss << "\n";
        }
        return oss.str();
    }
};

class Program {
    std::vector<std::unique_ptr<Stmt>> statements;
    std::string entryPointFunction;
    bool hasEntryPoint = false;

public:
    void addStatement(std::unique_ptr<Stmt> stmt) {
        statements.push_back(std::move(stmt));
    }
    
    void setEntryPointFunction(const std::string& name) {
        if (hasEntryPoint) {
            throw std::runtime_error("Only one @entrypoint allowed per program");
        }
        entryPointFunction = name;
        hasEntryPoint = true;
    }
    
    const std::string& getEntryPointFunction() const { 
        return entryPointFunction; 
    }
    
    bool getHasEntryPoint() const {
        return hasEntryPoint;
    }
    
    llvm::Value* codegen(::CodeGen& context);
    
    const std::vector<std::unique_ptr<Stmt>>& getStatements() const { 
        return statements; 
    }
    
    std::string toString(int indent = 0) const {
        std::ostringstream oss;
        oss << indentStr(indent) << "Program (" << statements.size() << " stmt(s))";
        if (hasEntryPoint) {
            oss << " [EntryPoint: " << entryPointFunction << "]";
        }
        oss << "\n";
        for (const auto& stmt : statements)
            oss << stmt->toString(indent + 1) << "\n";
        return oss.str();
    }
};

}
