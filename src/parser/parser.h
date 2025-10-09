#pragma once
#include "lexer/lexer.h"
#include "ast/ast.h"
#include "utils/error_utils.h"
#include <vector>
#include <memory>
#include <stdexcept>
#include <string>

class Parser {
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
    std::unique_ptr<AST::Expr> parseCallExpression();


    std::unique_ptr<AST::Expr> parseExpressionFromString(const std::string& exprStr);
    std::vector<std::unique_ptr<AST::Expr>> extractExpressionsFromFormat(const std::string& formatStr);

    std::unique_ptr<AST::Stmt> parseStatement();
    std::unique_ptr<AST::Stmt> parseVariableDeclaration();
    std::unique_ptr<AST::Stmt> parseAssignment();

    AST::VarType parseType();

    std::string getSourceLine(size_t line);
    void error(const std::string& msg);
    void errorAt(const Token& tok, const std::string& msg);

public:
    Parser(std::vector<Token> tokens, const std::string& source);
    std::unique_ptr<AST::Program> parse();
};
