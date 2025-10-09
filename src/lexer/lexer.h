#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <stdexcept>

enum class TokenType {
    // Keywords
    VAR,
    CONST,
    AS,
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

    std::string toString() const {
        static const std::unordered_map<TokenType, std::string> tokenTypeNames = {
            {TokenType::VAR, "VAR"},
            {TokenType::CONST, "CONST"},
            {TokenType::AS, "AS"},
            {TokenType::INT8, "INT8"},
            {TokenType::INT16, "INT16"},
            {TokenType::INT32, "INT32"},
            {TokenType::INT64, "INT64"},
            {TokenType::INT4, "INT4"},
            {TokenType::INT12, "INT12"},
            {TokenType::INT24, "INT24"},
            {TokenType::INT48, "INT48"},
            {TokenType::UINT0, "UINT0"},
            {TokenType::UINT4, "UINT4"},
            {TokenType::UINT12, "UINT12"},
            {TokenType::UINT24, "UINT24"},
            {TokenType::UINT48, "UINT48"},
            {TokenType::UINT8, "UINT8"},
            {TokenType::UINT16, "UINT16"},
            {TokenType::UINT32, "UINT32"},
            {TokenType::UINT64, "UINT64"},
            {TokenType::FLOAT32, "FLOAT32"},
            {TokenType::FLOAT64, "FLOAT64"},
            {TokenType::STRING, "STRING"},
            {TokenType::BOOL, "BOOL"},
            {TokenType::TRUE, "TRUE"},
            {TokenType::FALSE, "FALSE"},
            {TokenType::IDENTIFIER, "IDENTIFIER"},
            {TokenType::NUMBER, "NUMBER"},
            {TokenType::STRING_LITERAL, "STRING_LITERAL"},
            {TokenType::FLOAT_LITERAL, "FLOAT_LITERAL"},
            {TokenType::BUILTIN, "BUILTIN"},
            {TokenType::EQUALS, "EQUALS"},
            {TokenType::PLUS, "PLUS"},
            {TokenType::MINUS, "MINUS"},
            {TokenType::STAR, "STAR"},
            {TokenType::SLASH, "SLASH"},
            {TokenType::DOT, "DOT"},
            {TokenType::LESS, "LESS"},
            {TokenType::GREATER, "GREATER"},
            {TokenType::COLON, "COLON"},
            {TokenType::SEMICOLON, "SEMICOLON"},
            {TokenType::LPAREN, "LPAREN"},
            {TokenType::RPAREN, "RPAREN"},
            {TokenType::COMMA, "COMMA"},
            {TokenType::BACKTICK_STRING, "BACKTICK_STRING"},
            {TokenType::LBRACE, "LBRACE"},
            {TokenType::RBRACE, "RBRACE"},
            {TokenType::END_OF_FILE, "END_OF_FILE"},
            {TokenType::UNKNOWN, "UNKNOWN"}
        };

        auto it = tokenTypeNames.find(type);
        std::string typeStr = (it != tokenTypeNames.end()) ? it->second : "UNKNOWN";

        std::ostringstream oss;
        oss << typeStr << "('" << value << "') at line " << line << ", column " << column;
        return oss.str();
    }
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
