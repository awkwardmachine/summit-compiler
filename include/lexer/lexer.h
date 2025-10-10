#pragma once
#include "token.h"
#include <vector>
#include <string>
#include <unordered_map>

class Lexer {
private:
    std::string input;
    size_t position;
    size_t line;
    size_t column;
    
    static std::unordered_map<std::string, TokenType> keywords;
    
    char peek();
    char advance();
    void skipWhitespace();
    void skipComment();
    Token readNumber();
    Token readString();
    Token readBacktickString();
    Token readIdentifier();
    Token readBuiltin();

public:
    Lexer(const std::string& input);
    std::vector<Token> tokenize();
};