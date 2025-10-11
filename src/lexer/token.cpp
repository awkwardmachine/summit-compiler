#include "token.h"
#include <sstream>

Token::Token(TokenType t, const std::string& v, size_t l, size_t c)
    : type(t), value(v), line(l), column(c) {}

std::string Token::toString() const {
    static const std::unordered_map<TokenType, std::string> tokenTypeNames = {
        {TokenType::VAR, "VAR"}, {TokenType::CONST, "CONST"}, {TokenType::AS, "AS"},
        {TokenType::IF, "IF"}, {TokenType::THEN, "THEN"}, {TokenType::ELSE, "ELSE"},
        {TokenType::AND, "AND"}, {TokenType::OR, "OR"}, {TokenType::NOT, "NOT"},
        {TokenType::END, "END"}, {TokenType::BOOL, "BOOL"}, {TokenType::TRUE, "TRUE"},
        {TokenType::FALSE, "FALSE"}, {TokenType::FUNC, "FUNC"}, {TokenType::RETURN, "RET"},
        {TokenType::ENTRYPOINT, "ENTRYPOINT"}, {TokenType::WHILE, "WHILE"},
        
        {TokenType::INT4, "INT4"}, {TokenType::INT8, "INT8"}, {TokenType::INT12, "INT12"},
        {TokenType::INT16, "INT16"}, {TokenType::INT24, "INT24"}, {TokenType::INT32, "INT32"},
        {TokenType::INT48, "INT48"}, {TokenType::INT64, "INT64"},
        
        {TokenType::UINT0, "UINT0"}, {TokenType::UINT4, "UINT4"}, {TokenType::UINT8, "UINT8"},
        {TokenType::UINT12, "UINT12"}, {TokenType::UINT16, "UINT16"}, {TokenType::UINT24, "UINT24"},
        {TokenType::UINT32, "UINT32"}, {TokenType::UINT48, "UINT48"}, {TokenType::UINT64, "UINT64"},
        
        {TokenType::FLOAT32, "FLOAT32"}, {TokenType::FLOAT64, "FLOAT64"},
        {TokenType::STRING, "STRING"},
        
        {TokenType::IDENTIFIER, "IDENTIFIER"}, {TokenType::NUMBER, "NUMBER"},
        {TokenType::STRING_LITERAL, "STRING_LITERAL"}, {TokenType::FLOAT_LITERAL, "FLOAT_LITERAL"},
        {TokenType::BUILTIN, "BUILTIN"},
        
        {TokenType::EQUALS, "EQUALS"}, {TokenType::PLUS, "PLUS"}, {TokenType::MINUS, "MINUS"},
        {TokenType::STAR, "STAR"}, {TokenType::SLASH, "SLASH"}, {TokenType::DOT, "DOT"},
        {TokenType::PERCENT, "PERCENT"},
        
        {TokenType::LESS, "LESS"}, {TokenType::GREATER, "GREATER"},
        {TokenType::LESS_EQUAL, "LESS_EQUAL"}, {TokenType::GREATER_EQUAL, "GREATER_EQUAL"},
        {TokenType::EQUAL_EQUAL, "EQUAL_EQUAL"}, {TokenType::NOT_EQUAL, "NOT_EQUAL"},
        
        {TokenType::AND_AND, "AND_AND"}, {TokenType::OR_OR, "OR_OR"}, {TokenType::EXCLAMATION, "EXCLAMATION"},
        
        {TokenType::AMPERSAND, "AMPERSAND"}, {TokenType::PIPE, "PIPE"}, {TokenType::CARET, "CARET"},
        {TokenType::TILDE, "TILDE"}, {TokenType::LEFT_SHIFT, "LEFT_SHIFT"}, {TokenType::RIGHT_SHIFT, "RIGHT_SHIFT"},
        
        {TokenType::COLON, "COLON"}, {TokenType::SEMICOLON, "SEMICOLON"}, {TokenType::LPAREN, "LPAREN"},
        {TokenType::RPAREN, "RPAREN"}, {TokenType::COMMA, "COMMA"}, {TokenType::BACKTICK_STRING, "BACKTICK_STRING"},
        {TokenType::LBRACE, "LBRACE"}, {TokenType::RBRACE, "RBRACE"},
        
        {TokenType::END_OF_FILE, "END_OF_FILE"}, {TokenType::UNKNOWN, "UNKNOWN"}
    };

    auto it = tokenTypeNames.find(type);
    std::string typeStr = (it != tokenTypeNames.end()) ? it->second : "UNKNOWN";

    std::ostringstream oss;
    oss << typeStr << "('" << value << "') at line " << line << ", column " << column;
    return oss.str();
}