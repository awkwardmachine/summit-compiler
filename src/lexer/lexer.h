#pragma once
#include <string>
#include <vector>

enum class TokenType {
    // Keywords
    VAR,
    CONST,
    INT8,
    INT16,
    INT32,
    INT64,
    UINT0,
    STRING,
    
    // Literals
    IDENTIFIER,
    NUMBER,
    STRING_LITERAL,
    BUILTIN,
    
    // Operators
    EQUALS,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    
    // Punctuation
    COLON,
    SEMICOLON,
    LPAREN,
    RPAREN,
    COMMA,
    BACKTICK_STRING,  // For `format string with {}`
    LBRACE,           // {
    RBRACE,           // }
    
    // Special tokens
    END_OF_FILE,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    size_t line;
    size_t column;
    
    Token(TokenType t, const std::string& v, size_t l, size_t c)
        : type(t), value(v), line(l), column(c) {}
};

class Lexer {
    std::string input;
    size_t position;
    size_t line;
    size_t column;
    
    char peek();
    char advance();
    void skipWhitespace();
    void skipComment();
    Token readNumber();
    Token readString();
    Token readIdentifier();
    Token readBuiltin();
    Token readBacktickString();
    
public:
    Lexer(const std::string& input);
    std::vector<Token> tokenize();
};