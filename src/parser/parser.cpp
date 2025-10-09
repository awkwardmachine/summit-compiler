#include "parser.h"
#include <stdexcept>
#include <sstream>
#include <vector>

using namespace std;
using namespace AST;

Parser::Parser(vector<Token> tokens, const string& source)
    : tokens(tokens), current(0), source(source) {}

const Token& Parser::peek() { return tokens[current]; }
const Token& Parser::advance() { if (!isAtEnd()) current++; return tokens[current - 1]; }
bool Parser::match(TokenType type) { if (check(type)) { advance(); return true; } return false; }
bool Parser::check(TokenType type) { return !isAtEnd() && peek().type == type; }
bool Parser::checkNext(TokenType type) { return current + 1 < tokens.size() && tokens[current + 1].type == type; }
bool Parser::isAtEnd() { return peek().type == TokenType::END_OF_FILE; }

string Parser::getSourceLine(size_t line) {
    istringstream iss(source);
    string currentLine;
    size_t lineNum = 1;
    
    while (getline(iss, currentLine)) {
        if (lineNum == line) {
            return currentLine;
        }
        lineNum++;
    }
    
    return "";
}

void Parser::error(const string& msg) {
    const Token& tok = peek();
    string sourceLine = getSourceLine(tok.line);
    throw SyntaxError(msg, tok.line, tok.column, sourceLine);
}

void Parser::errorAt(const Token& tok, const string& msg) {
    string sourceLine = getSourceLine(tok.line);
    throw SyntaxError(msg, tok.line, tok.column, sourceLine);
}

unique_ptr<Expr> Parser::parseExpression() {
    return parseBinaryExpression(0);
}

AST::VarType Parser::parseType() {
    if (match(TokenType::BOOL)) return VarType::BOOL;
    if (match(TokenType::INT4)) return VarType::INT4;
    if (match(TokenType::INT8)) return VarType::INT8;
    if (match(TokenType::INT12)) return VarType::INT12;
    if (match(TokenType::INT16)) return VarType::INT16;
    if (match(TokenType::INT24)) return VarType::INT24;
    if (match(TokenType::INT32)) return VarType::INT32;
    if (match(TokenType::INT48)) return VarType::INT48;
    if (match(TokenType::INT64)) return VarType::INT64;
    if (match(TokenType::UINT4)) return VarType::UINT4;
    if (match(TokenType::UINT8)) return VarType::UINT8;
    if (match(TokenType::UINT12)) return VarType::UINT12;
    if (match(TokenType::UINT16)) return VarType::UINT16;
    if (match(TokenType::UINT24)) return VarType::UINT24;
    if (match(TokenType::UINT32)) return VarType::UINT32;
    if (match(TokenType::UINT48)) return VarType::UINT48;
    if (match(TokenType::UINT64)) return VarType::UINT64;
    if (match(TokenType::UINT0)) return VarType::UINT0;
    if (match(TokenType::STRING)) return VarType::STRING;
    error("Expected type");
    return VarType::INT32;
}

unique_ptr<Expr> Parser::parseExpressionFromString(const string& exprStr) {
    Lexer tempLexer(exprStr);
    auto tempTokens = tempLexer.tokenize();
    Parser tempParser(tempTokens, exprStr);
    return tempParser.parseExpression();
}

unique_ptr<Expr> Parser::parsePrimary() {
    const Token& tok = peek();
    
    if (match(TokenType::NUMBER)) return make_unique<NumberExpr>(tokens[current - 1].value);
    if (match(TokenType::STRING_LITERAL)) return make_unique<StringExpr>(tokens[current - 1].value);
    if (match(TokenType::BACKTICK_STRING)) return make_unique<StringExpr>(tokens[current - 1].value);

    if (match(TokenType::TRUE)) return make_unique<BooleanExpr>(true);
    if (match(TokenType::FALSE)) return make_unique<BooleanExpr>(false);

    if (match(TokenType::IDENTIFIER)) {
        string name = tokens[current - 1].value;
        if (match(TokenType::LPAREN)) {
            vector<unique_ptr<Expr>> args;
            if (!check(TokenType::RPAREN)) do { args.push_back(parseExpression()); } while(match(TokenType::COMMA));
            if (!match(TokenType::RPAREN)) error("Expected ')' after function arguments");
            return make_unique<CallExpr>(name, move(args));
        }
        return make_unique<VariableExpr>(name);
    }

    
    if (match(TokenType::BUILTIN)) {
        string name = tokens[current - 1].value;
        
        if (name == "print" || name == "println") {
            if (!match(TokenType::LPAREN)) {
                throw runtime_error("Expected '(' after builtin function");
            }
            
            vector<unique_ptr<Expr>> args;
            
            if (!check(TokenType::RPAREN)) {
                if (check(TokenType::BACKTICK_STRING)) {
                    auto stringToken = advance();
                    args.push_back(make_unique<StringExpr>(stringToken.value));
                    
                    auto expressions = extractExpressionsFromFormat(stringToken.value);
                    for (auto& expr : expressions) {
                        args.push_back(move(expr));
                    }
                } else {
                    args.push_back(parseExpression());
                }
                
                while (match(TokenType::COMMA)) {
                    args.push_back(parseExpression());
                }
            }
            
            if (!match(TokenType::RPAREN)) {
                throw runtime_error("Expected ')' after arguments");
            }
            return make_unique<CallExpr>("@" + name, move(args));
        } else {
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
    }

    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        if (!match(TokenType::RPAREN)) error("Expected ')' after expression");
        return expr;
    }

    string errorMsg = "Expected expression, but found: " + tok.value;
    error(errorMsg);
    return nullptr;
}

vector<unique_ptr<Expr>> Parser::extractExpressionsFromFormat(const string& formatStr) {
    vector<unique_ptr<Expr>> expressions;
    
    size_t pos = 0;
    while ((pos = formatStr.find('{', pos)) != std::string::npos) {
        size_t endPos = formatStr.find('}', pos);
        if (endPos == std::string::npos) {
            throw std::runtime_error("Unclosed '{' in format string");
        }
        
        string exprStr = formatStr.substr(pos + 1, endPos - pos - 1);
        
        try {
            auto expr = parseExpressionFromString(exprStr);
            expressions.push_back(move(expr));
        } catch (const exception& e) {
            throw runtime_error("Invalid expression in format string: " + exprStr + " - " + e.what());
        }
        
        pos = endPos + 1;
    }
    
    return expressions;
}

unique_ptr<Expr> Parser::parseBinaryExpression(int minPrecedence) {
    auto left = parsePrimary();
    while (true) {
        BinaryOp op;
        int precedence = 0;
        
        if (check(TokenType::PLUS)) { op = BinaryOp::ADD; precedence = 1; }
        else if (check(TokenType::MINUS)) { op = BinaryOp::SUBTRACT; precedence = 1; }
        else if (check(TokenType::STAR)) { op = BinaryOp::MULTIPLY; precedence = 2; }
        else if (check(TokenType::SLASH)) { op = BinaryOp::DIVIDE; precedence = 2; }
        else break;

        if (precedence < minPrecedence) break;
        
        advance();
        auto right = parseBinaryExpression(precedence + 1);
        left = make_unique<BinaryExpr>(op, move(left), move(right));
    }
    return left;
}

unique_ptr<Stmt> Parser::parseVariableDeclaration() {
    bool isConst = match(TokenType::CONST);
    if (!isConst && !match(TokenType::VAR)) error("Expected 'var' or 'const'");
    if (!match(TokenType::IDENTIFIER)) error("Expected variable name");
    string name = tokens[current - 1].value;
    if (!match(TokenType::COLON)) error("Expected ':' after variable name");
    VarType type = parseType();
    unique_ptr<Expr> value = nullptr;
    if (match(TokenType::EQUALS)) value = parseExpression();
    
    const Token& lastToken = tokens[current - 1];
    if (!match(TokenType::SEMICOLON)) {
        errorAt(lastToken, "Expected ';' after variable declaration");
    }
    return make_unique<VariableDecl>(name, type, isConst, move(value));
}

unique_ptr<Stmt> Parser::parseAssignment() {
    string name = tokens[current - 1].value;
    if (!match(TokenType::EQUALS)) error("Expected '=' after variable name");
    auto value = parseExpression();
    
    const Token& lastToken = tokens[current - 1];
    if (!match(TokenType::SEMICOLON)) {
        errorAt(lastToken, "Expected ';' after assignment");
    }
    return make_unique<AssignmentStmt>(name, move(value));
}

unique_ptr<Stmt> Parser::parseStatement() {
    if (check(TokenType::VAR) || check(TokenType::CONST)) return parseVariableDeclaration();
    if (check(TokenType::IDENTIFIER) && checkNext(TokenType::EQUALS)) { advance(); return parseAssignment(); }
    auto expr = parseExpression();
    
    const Token& lastToken = tokens[current - 1];
    if (!match(TokenType::SEMICOLON)) {
        errorAt(lastToken, "Expected ';' after expression");
    }
    return make_unique<ExprStmt>(move(expr));
}

unique_ptr<Program> Parser::parse() {
    auto program = make_unique<Program>();
    while (!isAtEnd()) program->addStatement(parseStatement());
    return program;
}