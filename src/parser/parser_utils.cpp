#include "parser.h"
#include <sstream>

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
        if (lineNum == line) return currentLine;
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
    if (match(TokenType::FLOAT32)) return VarType::FLOAT32;
    if (match(TokenType::FLOAT64)) return VarType::FLOAT64;
    if (match(TokenType::STRING)) return VarType::STRING;
    error("Expected type");
    return VarType::VOID;
}

unique_ptr<Expr> Parser::parseExpressionFromString(const string& exprStr) {
    Lexer tempLexer(exprStr);
    auto tempTokens = tempLexer.tokenize();
    Parser tempParser(tempTokens, exprStr);
    return tempParser.parseExpression();
}

string Parser::buildFormatSpecifiers(const string& formatStr) {
    string formatSpecifiers;
    size_t pos = 0;
    size_t lastPos = 0;
    
    while ((pos = formatStr.find('{', lastPos)) != string::npos) {
        if (pos > lastPos) {
            formatSpecifiers += formatStr.substr(lastPos, pos - lastPos);
        }
        
        size_t endPos = formatStr.find('}', pos);
        if (endPos == string::npos) {
            throw runtime_error("Unclosed '{' in format string");
        }
        
        formatSpecifiers += "%s";
        lastPos = endPos + 1;
    }

    if (lastPos < formatStr.length()) {
        formatSpecifiers += formatStr.substr(lastPos);
    }
    
    return formatSpecifiers;
}

vector<unique_ptr<Expr>> Parser::extractExpressionsFromFormat(const string& formatStr) {
    vector<unique_ptr<Expr>> expressions;
    size_t pos = 0;
    
    while ((pos = formatStr.find('{', pos)) != string::npos) {
        size_t endPos = formatStr.find('}', pos);
        if (endPos == string::npos) {
            throw runtime_error("Unclosed '{' in format string");
        }
        
        string exprStr = formatStr.substr(pos + 1, endPos - pos - 1);
        
        try {
            Lexer tempLexer(exprStr);
            auto tempTokens = tempLexer.tokenize();
            Parser tempParser(tempTokens, exprStr);
            auto expr = tempParser.parseExpression();
            expressions.push_back(move(expr));
        } catch (const exception& e) {
            throw runtime_error("Invalid expression in format string: " + exprStr + " - " + e.what());
        }
        
        pos = endPos + 1;
    }
    
    return expressions;
}