#pragma once
#include "lexer/lexer.h"
#include "ast/ast.h"
#include "utils/error_utils.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_set>

class Parser {
private:
    std::vector<Token> tokens;
    size_t current;
    std::string source;

    const Token& peek();
    const Token& advance();
    bool match(TokenType type);
    bool check(TokenType type);
    bool checkNext(TokenType type);
    bool isAtEnd();

    std::unique_ptr<AST::Expr> parseExpression();
    std::unique_ptr<AST::Expr> parsePrimary();
    std::unique_ptr<AST::Expr> parseBinaryExpression(int minPrecedence);
    std::unique_ptr<AST::Expr> parseCastExpression();
    std::unique_ptr<AST::Expr> parseUnaryExpression();
    std::unique_ptr<AST::Stmt> parseIfStatement();
    std::unique_ptr<AST::Stmt> parseFunctionDeclaration();
    std::unique_ptr<AST::Stmt> parseReturnStatement();
    std::unique_ptr<AST::Stmt> parseBlock();
    std::unique_ptr<AST::Stmt> parseElseIfChain();
    std::unique_ptr<AST::Stmt> parseEntrypointStatement();
    std::unique_ptr<AST::Stmt> parseWhileStatement();
    std::unique_ptr<AST::Stmt> parseAssignmentOrIncrement();
    std::unique_ptr<AST::Stmt> parseForLoopStatement();
    std::unique_ptr<AST::Expr> parseBuiltinCall();
    std::unique_ptr<AST::Stmt> parseEnumDeclaration();
    std::unique_ptr<AST::Expr> parseMemberAccess(std::unique_ptr<AST::Expr> object);

    std::unique_ptr<AST::Expr> parseExpressionFromString(const std::string& exprStr);
    std::vector<std::unique_ptr<AST::Expr>> extractExpressionsFromFormat(const std::string& formatStr);
    std::string buildFormatSpecifiers(const std::string& formatStr);

    std::unique_ptr<AST::Stmt> parseStatement();
    std::unique_ptr<AST::Stmt> parseVariableDeclaration();
    std::unique_ptr<AST::Stmt> parseAssignment();

    AST::VarType parseType();

    std::string getSourceLine(size_t line);
    void error(const std::string& msg);
    void errorAt(const Token& tok, const std::string& msg);
    void registerEnumType(const std::string& name) {
        enumTypes.insert(name);
    }
    
    bool isEnumType(const std::string& name) {
        return enumTypes.find(name) != enumTypes.end();
    }

public:
    Parser(std::vector<Token> tokens, const std::string& source);
    std::unique_ptr<AST::Program> parse();
    std::unordered_set<std::string> enumTypes;
};