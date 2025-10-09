#include "parser.h"
#include <stdexcept>

using namespace std;
using namespace AST;

Parser::Parser(vector<Token> tokens) 
    : tokens(tokens), current(0) {}

const Token& Parser::peek() {
    return tokens[current];
}

const Token& Parser::advance() {
    if (!isAtEnd()) current++;
    return tokens[current - 1];
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::checkNext(TokenType type) {
    if (current + 1 >= tokens.size()) return false;
    return tokens[current + 1].type == type;
}

bool Parser::isAtEnd() {
    return peek().type == TokenType::END_OF_FILE;
}

VarType Parser::parseType() {
    if (match(TokenType::INT8)) {
        return VarType::INT8;
    } else if (match(TokenType::INT16)) {
        return VarType::INT16;
    } else if (match(TokenType::INT32)) {
        return VarType::INT32;
    } else if (match(TokenType::INT64)) {
        return VarType::INT64;
    } else if (match(TokenType::STRING)) {
        return VarType::STRING;
    }
    throw runtime_error("Expected type");
}

unique_ptr<Expr> Parser::parsePrimary() {
    if (match(TokenType::NUMBER)) {
        try {
            return make_unique<NumberExpr>(tokens[current - 1].value);
        } catch (const std::runtime_error& e) {
            throw runtime_error("Invalid number format: " + tokens[current - 1].value);
        }
    }
    
    if (match(TokenType::STRING_LITERAL)) {
        return make_unique<StringExpr>(tokens[current - 1].value);
    }
    
    if (match(TokenType::IDENTIFIER)) {
        string name = tokens[current - 1].value;
        
        if (match(TokenType::LPAREN)) {
            vector<unique_ptr<Expr>> args;
            if (!check(TokenType::RPAREN)) {
                do {
                    args.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }
            if (!match(TokenType::RPAREN)) {
                throw runtime_error("Expected ')' after arguments");
            }
            return make_unique<CallExpr>(name, move(args));
        }
        
        return make_unique<VariableExpr>(name);
    }
    
    if (match(TokenType::BUILTIN)) {
        string name = tokens[current - 1].value;
        
        if (!match(TokenType::LPAREN)) {
            throw runtime_error("Expected '(' after builtin function");
        }
        
        vector<unique_ptr<Expr>> args;
        if (!check(TokenType::RPAREN)) {
            do {
                args.push_back(parseExpression());
            } while (match(TokenType::COMMA));
        }
        if (!match(TokenType::RPAREN)) {
            throw runtime_error("Expected ')' after arguments");
        }
        return make_unique<CallExpr>("@" + name, move(args));
    }
    
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        if (!match(TokenType::RPAREN)) {
            throw runtime_error("Expected ')' after expression");
        }
        return expr;
    }
    
    throw runtime_error("Expected expression");
}

unique_ptr<Expr> Parser::parseExpression() {
    return parseBinaryExpression(0);
}

unique_ptr<Expr> Parser::parseBinaryExpression(int minPrecedence) {
    auto left = parsePrimary();
    
    while (true) {
        BinaryOp op;
        int precedence = 0;
        
        if (match(TokenType::PLUS)) {
            op = BinaryOp::ADD;
            precedence = 1;
        } else if (match(TokenType::MINUS)) {
            op = BinaryOp::SUBTRACT;
            precedence = 1;
        } else if (match(TokenType::STAR)) {
            op = BinaryOp::MULTIPLY;
            precedence = 2;
        } else if (match(TokenType::SLASH)) {
            op = BinaryOp::DIVIDE;
            precedence = 2;
        } else {
            break;
        }
        
        if (precedence < minPrecedence) {
            break;
        }
        
        auto right = parseBinaryExpression(precedence + 1);
        left = make_unique<BinaryExpr>(op, move(left), move(right));
    }
    
    return left;
}

unique_ptr<Stmt> Parser::parseVariableDeclaration() {
    bool isConst = match(TokenType::CONST);
    if (!isConst && !match(TokenType::VAR)) {
        throw runtime_error("Expected 'var' or 'const'");
    }
    
    if (!match(TokenType::IDENTIFIER)) {
        throw runtime_error("Expected variable name");
    }
    string name = tokens[current - 1].value;
    
    if (!match(TokenType::COLON)) {
        throw runtime_error("Expected ':' after variable name");
    }
    
    VarType type = parseType();
    
    unique_ptr<Expr> value = nullptr;
    if (match(TokenType::EQUALS)) {
        value = parseExpression();
    }
    
    if (!match(TokenType::SEMICOLON)) {
        throw runtime_error("Expected ';' after variable declaration");
    }
    
    return make_unique<VariableDecl>(name, type, isConst, move(value));
}

unique_ptr<Stmt> Parser::parseAssignment() {
    string name = tokens[current - 1].value;
    
    if (!match(TokenType::EQUALS)) {
        throw runtime_error("Expected '=' after variable name");
    }
    
    auto value = parseExpression();
    
    if (!match(TokenType::SEMICOLON)) {
        throw runtime_error("Expected ';' after assignment");
    }
    
    return make_unique<AssignmentStmt>(name, move(value));
}

unique_ptr<Stmt> Parser::parseStatement() {
    if (check(TokenType::VAR) || check(TokenType::CONST)) {
        return parseVariableDeclaration();
    }
    
    if (check(TokenType::IDENTIFIER) && checkNext(TokenType::EQUALS)) {
        advance();
        return parseAssignment();
    }
    
    auto expr = parseExpression();
    if (!match(TokenType::SEMICOLON)) {
        throw runtime_error("Expected ';' after expression");
    }
    return make_unique<ExprStmt>(move(expr));
}

unique_ptr<Program> Parser::parse() {
    auto program = make_unique<Program>();
    
    while (!isAtEnd()) {
        program->addStatement(parseStatement());
    }
    
    return program;
}