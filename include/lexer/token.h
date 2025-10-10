#pragma once
#include <string>
#include <unordered_map>

enum class TokenType {
    // Keywords
    VAR, CONST, AS, BOOL, TRUE, FALSE,
    THEN, ELSE, ELSEIF, END, IF, AND, OR, NOT,
    FUNC, RETURN,
    
    // Integer types
    INT4, INT8, INT12, INT16, INT24, INT32, INT48, INT64,
    UINT0, UINT4, UINT8, UINT12, UINT16, UINT24, UINT32, UINT48, UINT64,
    
    // Float types
    FLOAT32, FLOAT64,
    
    // String type
    STRING,
    
    // Literals
    IDENTIFIER, NUMBER, STRING_LITERAL, FLOAT_LITERAL, BUILTIN,
    
    // Operators
    EQUALS, PLUS, MINUS, STAR, SLASH, PERCENT, DOT,
    GREATER, LESS, GREATER_EQUAL, LESS_EQUAL, EQUAL_EQUAL, NOT_EQUAL,
    
    // Logical operators
    AND_AND, OR_OR, EXCLAMATION,
    
    // Bitwise operators
    AMPERSAND, PIPE, CARET, TILDE, LEFT_SHIFT, RIGHT_SHIFT,
    
    // Punctuation
    COLON, SEMICOLON, LPAREN, RPAREN, COMMA, BACKTICK_STRING, LBRACE, RBRACE,
    
    // Special tokens
    END_OF_FILE, UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    size_t line;
    size_t column;
    
    Token(TokenType t, const std::string& v, size_t l, size_t c);
    std::string toString() const;
};