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
    INT4,
    INT12,
    INT24,
    INT48,
    UINT0,
    UINT4,
    UINT12,
    UINT24,
    UINT48,
    UINT8,
    UINT16,
    UINT32,
    UINT64,
    FLOAT32,
    FLOAT64,
    STRING,
    BOOL,
    TRUE,
    FALSE,
    
    // Literals
    IDENTIFIER,
    NUMBER,
    STRING_LITERAL,
    FLOAT_LITERAL,
    BUILTIN,
    
    // Operators
    EQUALS,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    DOT,
    LESS,
    GREATER,
    
    // Punctuation
    COLON,
    SEMICOLON,
    LPAREN,
    RPAREN,
    COMMA,
    BACKTICK_STRING,
    LBRACE,
    RBRACE,
    
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