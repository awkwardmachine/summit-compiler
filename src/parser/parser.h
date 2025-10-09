#pragma once
#include "lexer/lexer.h"
#include "ast/ast.h"
#include <vector>
#include <memory>

class Parser {
    std::vector<Token> tokens;
    size_t current;
    
    const Token& peek();
    const Token& advance();
    bool match(TokenType type);
    bool check(TokenType type);
    bool checkNext(TokenType type);
    bool isAtEnd();
    
    std::unique_ptr<AST::Expr> parseExpression();
    std::unique_ptr<AST::Expr> parsePrimary();
    std::unique_ptr<AST::Expr> parseBinaryExpression(int minPrecedence);
    std::unique_ptr<AST::Stmt> parseStatement();
    std::unique_ptr<AST::Stmt> parseVariableDeclaration();
    std::unique_ptr<AST::Stmt> parseAssignment();
    std::unique_ptr<AST::Expr> parseCallExpression();
    AST::VarType parseType();
    
public:
    Parser(std::vector<Token> tokens);
    std::unique_ptr<AST::Program> parse();
};