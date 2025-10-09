#include "lexer/lexer.h"
#include <cctype>
#include <unordered_map>
#include <stdexcept>

using namespace std;

static unordered_map<string, TokenType> keywords = {
    {"var", TokenType::VAR},
    {"const", TokenType::CONST},
    {"bool", TokenType::BOOL},
    {"true", TokenType::TRUE},
    {"false", TokenType::FALSE},
    {"int4", TokenType::INT4},
    {"int8", TokenType::INT8},
    {"int12", TokenType::INT12},
    {"int16", TokenType::INT16},
    {"int24", TokenType::INT24},
    {"int32", TokenType::INT32},
    {"int48", TokenType::INT48},
    {"int64", TokenType::INT64},
    {"uint4", TokenType::UINT4},
    {"uint8", TokenType::UINT8},
    {"uint12", TokenType::UINT12},
    {"uint16", TokenType::UINT16},
    {"uint24", TokenType::UINT24},
    {"uint32", TokenType::UINT32},
    {"uint48", TokenType::UINT48},
    {"uint64", TokenType::UINT64},
    {"uint0", TokenType::UINT0},
    {"float32", TokenType::FLOAT32},
    {"float64", TokenType::FLOAT64},
    {"str", TokenType::STRING}
};

Lexer::Lexer(const string& input) 
    : input(input), position(0), line(1), column(1) {}

char Lexer::peek() {
    if (position >= input.length()) return '\0';
    return input[position];
}

char Lexer::advance() {
    if (position >= input.length()) return '\0';
    char c = input[position++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

void Lexer::skipWhitespace() {
    while (isspace(peek())) {
        advance();
    }
}

void Lexer::skipComment() {
    if (peek() == '/') {
        advance();
        if (peek() == '/') {
            while (peek() != '\n' && peek() != '\0') {
                advance();
            }
        } else if (peek() == '*') {
            advance();
            while (true) {
                if (peek() == '\0') break;
                if (peek() == '*') {
                    advance();
                    if (peek() == '/') {
                        advance();
                        break;
                    }
                } else {
                    advance();
                }
            }
        } else {
            position--;
        }
    }
}

Token Lexer::readNumber() {
    string number;
    size_t startLine = line, startCol = column;
    bool isFloat = false;
    bool hasExponent = false;
    
    while (isdigit(peek())) {
        number += advance();
    }

    if (peek() == '.') {
        isFloat = true;
        number += advance();
        
        while (isdigit(peek())) {
            number += advance();
        }
    }
    
    if (peek() == 'e' || peek() == 'E') {
        isFloat = true;
        hasExponent = true;
        number += advance();

        if (peek() == '+' || peek() == '-') {
            number += advance();
        }
        
        while (isdigit(peek())) {
            number += advance();
        }
    }
    
    if (isFloat) {
        if (number.empty() || 
            number == "." || 
            number.back() == 'e' || 
            number.back() == 'E' ||
            number.back() == '+' || 
            number.back() == '-') {
            throw runtime_error("Invalid float literal: " + number);
        }
        
        return Token(TokenType::FLOAT_LITERAL, number, startLine, startCol);
    } else {
        return Token(TokenType::NUMBER, number, startLine, startCol);
    }
}

Token Lexer::readString() {
    string str;
    size_t startLine = line, startCol = column;
    
    advance();
    
    while (peek() != '"' && peek() != '\0') {
        if (peek() == '\\') {
            advance();
            switch (peek()) {
                case 'n': str += '\n'; break;
                case 't': str += '\t'; break;
                case '\\': str += '\\'; break;
                case '"': str += '"'; break;
                default: str += '\\'; str += peek(); break;
            }
            advance();
        } else {
            str += advance();
        }
    }
    
    if (peek() != '"') {
        throw runtime_error("Unterminated string literal");
    }
    advance();
    
    return Token(TokenType::STRING_LITERAL, str, startLine, startCol);
}

Token Lexer::readBacktickString() {
    string str;
    size_t startLine = line, startCol = column;
    
    advance();
    
    while (peek() != '`' && peek() != '\0') {
        if (peek() == '\\') {
            advance();
            switch (peek()) {
                case 'n': str += '\n'; break;
                case 't': str += '\t'; break;
                case '\\': str += '\\'; break;
                case '`': str += '`'; break;
                case '{': str += '{'; break;
                case '}': str += '}'; break;
                default: str += '\\'; str += peek(); break;
            }
            advance();
        } else {
            str += advance();
        }
    }
    
    if (peek() != '`') {
        throw runtime_error("Unterminated backtick string literal");
    }
    advance();
    
    return Token(TokenType::BACKTICK_STRING, str, startLine, startCol);
}

Token Lexer::readIdentifier() {
    string ident;
    size_t startLine = line, startCol = column;
    
    while (isalnum(peek()) || peek() == '_') {
        ident += advance();
    }
    
    auto it = keywords.find(ident);
    if (it != keywords.end()) {
        return Token(it->second, ident, startLine, startCol);
    }
    
    return Token(TokenType::IDENTIFIER, ident, startLine, startCol);
}

Token Lexer::readBuiltin() {
    string builtin;
    size_t startLine = line, startCol = column;
    
    advance();
    
    while (isalnum(peek()) || peek() == '_') {
        builtin += advance();
    }
    
    return Token(TokenType::BUILTIN, builtin, startLine, startCol);
}

vector<Token> Lexer::tokenize() {
    vector<Token> tokens;
    
    while (position < input.length()) {
        skipWhitespace();

        if (peek() == '/') {
            char nextChar = input[position + 1];
            if (nextChar == '/' || nextChar == '*') {
                skipComment();
                continue;
            }
        }

        char c = peek();
        if (c == '\0') break;
        
        size_t currentLine = line, currentCol = column;
        
        if (isdigit(c) || (c == '.' && isdigit(input[position + 1]))) {
            tokens.push_back(readNumber());
        } else if (c == '"') {
            tokens.push_back(readString());
        } else if (c == '`') {
            tokens.push_back(readBacktickString());
        } else if (isalpha(c) || c == '_') {
            tokens.push_back(readIdentifier());
        } else if (c == '@') {
            tokens.push_back(readBuiltin());
        } else {
            switch (c) {
                case '=':
                    tokens.push_back(Token(TokenType::EQUALS, "=", currentLine, currentCol));
                    advance();
                    break;
                case '+':
                    tokens.push_back(Token(TokenType::PLUS, "+", currentLine, currentCol));
                    advance();
                    break;
                case '-':
                    tokens.push_back(Token(TokenType::MINUS, "-", currentLine, currentCol));
                    advance();
                    break;
                case '*':
                    tokens.push_back(Token(TokenType::STAR, "*", currentLine, currentCol));
                    advance();
                    break;
                case '/':
                    tokens.push_back(Token(TokenType::SLASH, "/", currentLine, currentCol));
                    advance();
                    break;
                case ':':
                    tokens.push_back(Token(TokenType::COLON, ":", currentLine, currentCol));
                    advance();
                    break;
                case ';':
                    tokens.push_back(Token(TokenType::SEMICOLON, ";", currentLine, currentCol));
                    advance();
                    break;
                case '(':
                    tokens.push_back(Token(TokenType::LPAREN, "(", currentLine, currentCol));
                    advance();
                    break;
                case ')':
                    tokens.push_back(Token(TokenType::RPAREN, ")", currentLine, currentCol));
                    advance();
                    break;
                case ',':
                    tokens.push_back(Token(TokenType::COMMA, ",", currentLine, currentCol));
                    advance();
                    break;
                case '{':
                    tokens.push_back(Token(TokenType::LBRACE, "{", currentLine, currentCol));
                    advance();
                    break;
                case '}':
                    tokens.push_back(Token(TokenType::RBRACE, "}", currentLine, currentCol));
                    advance();
                    break;
                case '.':
                    tokens.push_back(Token(TokenType::DOT, ".", currentLine, currentCol));
                    advance();
                    break;
                case '<':
                    tokens.push_back(Token(TokenType::LESS, "<", currentLine, currentCol));
                    advance();
                    break;
                case '>':
                    tokens.push_back(Token(TokenType::GREATER, ">", currentLine, currentCol));
                    advance();
                    break;
                default:
                    tokens.push_back(Token(TokenType::UNKNOWN, string(1, c), currentLine, currentCol));
                    advance();
                    break;
            }
        }
    }
    
    tokens.push_back(Token(TokenType::END_OF_FILE, "", line, column));
    return tokens;
}