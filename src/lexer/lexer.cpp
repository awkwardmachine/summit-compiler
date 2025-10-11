#include "lexer.h"
#include <cctype>
#include <stdexcept>

using namespace std;

unordered_map<string, TokenType> Lexer::keywords = {
    {"var", TokenType::VAR}, {"const", TokenType::CONST}, {"as", TokenType::AS},
    {"bool", TokenType::BOOL}, {"true", TokenType::TRUE}, {"false", TokenType::FALSE},
    {"if", TokenType::IF}, {"then", TokenType::THEN}, {"else", TokenType::ELSE},
    {"elseif", TokenType::ELSEIF}, {"end", TokenType::END}, {"and", TokenType::AND},
    {"or", TokenType::OR}, {"not", TokenType::NOT}, {"func", TokenType::FUNC},
    {"ret", TokenType::RETURN}, {"while", TokenType::WHILE},
    
    {"int4", TokenType::INT4}, {"int8", TokenType::INT8}, {"int12", TokenType::INT12},
    {"int16", TokenType::INT16}, {"int24", TokenType::INT24}, {"int32", TokenType::INT32},
    {"int48", TokenType::INT48}, {"int64", TokenType::INT64},
    
    {"uint0", TokenType::UINT0}, {"uint4", TokenType::UINT4}, {"uint8", TokenType::UINT8},
    {"uint12", TokenType::UINT12}, {"uint16", TokenType::UINT16}, {"uint24", TokenType::UINT24},
    {"uint32", TokenType::UINT32}, {"uint48", TokenType::UINT48}, {"uint64", TokenType::UINT64},
    
    {"float32", TokenType::FLOAT32}, {"float64", TokenType::FLOAT64}, {"str", TokenType::STRING}
};

Lexer::Lexer(const string& input) 
    : input(input), position(0), line(1), column(1) {}

char Lexer::peek() {
    return position >= input.length() ? '\0' : input[position];
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
    while (isspace(peek())) advance();
}

void Lexer::skipComment() {
    if (peek() == '/') {
        advance();
        if (peek() == '/') {
            while (peek() != '\n' && peek() != '\0') advance();
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
    
    if (peek() == '0' && (input[position + 1] == 'b' || input[position + 1] == 'B')) {
        advance(); advance();
        number = "0b";
        while (peek() == '0' || peek() == '1' || peek() == '_') {
            if (peek() != '_') number += advance();
            else advance();
        }
        if (number.length() <= 2) throw runtime_error("Invalid binary literal: " + number);
        return Token(TokenType::NUMBER, number, startLine, startCol);
    }
    
    if (peek() == '0' && (input[position + 1] == 'x' || input[position + 1] == 'X')) {
        advance(); advance();
        number = "0x";
        while (isxdigit(peek()) || peek() == '_') {
            if (peek() != '_') number += advance();
            else advance();
        }
        if (number.length() <= 2) throw runtime_error("Invalid hex literal: " + number);
        return Token(TokenType::NUMBER, number, startLine, startCol);
    }

    while (isdigit(peek()) || peek() == '_') {
        if (peek() != '_') number += advance();
        else advance();
    }

    if (peek() == '.') {
        isFloat = true;
        number += advance();
        while (isdigit(peek()) || peek() == '_') {
            if (peek() != '_') number += advance();
            else advance();
        }
    }
    
    if (peek() == 'e' || peek() == 'E') {
        isFloat = true;
        number += advance();
        if (peek() == '+' || peek() == '-') number += advance();
        while (isdigit(peek()) || peek() == '_') {
            if (peek() != '_') number += advance();
            else advance();
        }
    }
    
    if (isFloat) {
        if (number.empty() || number == "." || number.back() == 'e' || 
            number.back() == 'E' || number.back() == '+' || number.back() == '-') {
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
    
    if (peek() != '"') throw runtime_error("Unterminated string literal");
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
    
    if (peek() != '`') throw runtime_error("Unterminated backtick string literal");
    advance();
    return Token(TokenType::BACKTICK_STRING, str, startLine, startCol);
}

Token Lexer::readIdentifier() {
    string ident;
    size_t startLine = line, startCol = column;
    
    while (isalnum(peek()) || peek() == '_') ident += advance();
    
    auto it = keywords.find(ident);
    return it != keywords.end() 
        ? Token(it->second, ident, startLine, startCol)
        : Token(TokenType::IDENTIFIER, ident, startLine, startCol);
}

Token Lexer::readBuiltin() {
    string builtin;
    size_t startLine = line, startCol = column;
    advance();
    
    if (!isalpha(peek()) && peek() != '_') {
        throw runtime_error("Expected identifier after '@'");
    }

    while (isalnum(peek()) || peek() == '_') builtin += advance();
    
    if (builtin == "entrypoint") {
        return Token(TokenType::ENTRYPOINT, "@entrypoint", startLine, startCol);
    }
    
    return Token(TokenType::BUILTIN, "@" + builtin, startLine, startCol);
}
vector<Token> Lexer::tokenize() {
    vector<Token> tokens;
    
    while (position < input.length()) {
        skipWhitespace();
        if (peek() == '/' && (input[position + 1] == '/' || input[position + 1] == '*')) {
            skipComment();
            continue;
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
                    advance(); 
                    if (peek() == '=') {
                        advance(); 
                        tokens.push_back(Token(TokenType::EQUAL_EQUAL, "==", currentLine, currentCol));
                    } else {
                        tokens.push_back(Token(TokenType::EQUALS, "=", currentLine, currentCol));
                    }
                    break;
                case '!': 
                    advance(); 
                    tokens.push_back(peek() == '=' 
                        ? (advance(), Token(TokenType::NOT_EQUAL, "!=", currentLine, currentCol))
                        : Token(TokenType::EXCLAMATION, "!", currentLine, currentCol)); 
                    break;
                case '<': 
                    advance(); 
                    if (peek() == '=') {
                        advance(); 
                        tokens.push_back(Token(TokenType::LESS_EQUAL, "<=", currentLine, currentCol));
                    } else if (peek() == '<') {
                        advance(); 
                        tokens.push_back(Token(TokenType::LEFT_SHIFT, "<<", currentLine, currentCol));
                    } else {
                        tokens.push_back(Token(TokenType::LESS, "<", currentLine, currentCol));
                    } 
                    break;
                case '>': 
                    advance();
                    if (peek() == '=') {
                        advance(); 
                        tokens.push_back(Token(TokenType::GREATER_EQUAL, ">=", currentLine, currentCol));
                    } else if (peek() == '>') {
                        advance(); 
                        tokens.push_back(Token(TokenType::RIGHT_SHIFT, ">>", currentLine, currentCol));
                    } else {
                        tokens.push_back(Token(TokenType::GREATER, ">", currentLine, currentCol));
                    } 
                    break;
                case '&': 
                    advance(); 
                    tokens.push_back(peek() == '&' 
                        ? (advance(), Token(TokenType::AND_AND, "&&", currentLine, currentCol))
                        : Token(TokenType::AMPERSAND, "&", currentLine, currentCol)); 
                    break;
                case '|': 
                    advance(); 
                    tokens.push_back(peek() == '|' 
                        ? (advance(), Token(TokenType::OR_OR, "||", currentLine, currentCol))
                        : Token(TokenType::PIPE, "|", currentLine, currentCol)); 
                    break;
                case '^': 
                    tokens.push_back(Token(TokenType::CARET, "^", currentLine, currentCol)); 
                    advance(); 
                    break;
                case '~': 
                    tokens.push_back(Token(TokenType::TILDE, "~", currentLine, currentCol)); 
                    advance(); 
                    break;
                case '%': 
                    tokens.push_back(Token(TokenType::PERCENT, "%", currentLine, currentCol)); 
                    advance(); 
                    break;
                case '+': 
                    advance();
                    if (peek() == '=') {
                        advance();
                        tokens.push_back(Token(TokenType::PLUS_EQUALS, "+=", currentLine, currentCol));
                    } else if (peek() == '+') {
                        advance();
                        tokens.push_back(Token(TokenType::INCREMENT, "++", currentLine, currentCol));
                    } else {
                        tokens.push_back(Token(TokenType::PLUS, "+", currentLine, currentCol));
                    }
                    break;
                case '-': 
                    advance();
                    if (peek() == '=') {
                        advance();
                        tokens.push_back(Token(TokenType::MINUS_EQUALS, "-=", currentLine, currentCol));
                    } else if (peek() == '-') {
                        advance();
                        tokens.push_back(Token(TokenType::DECREMENT, "--", currentLine, currentCol));
                    } else {
                        tokens.push_back(Token(TokenType::MINUS, "-", currentLine, currentCol));
                    }
                    break;
                case '*': 
                    advance();
                    if (peek() == '=') {
                        advance();
                        tokens.push_back(Token(TokenType::STAR_EQUALS, "*=", currentLine, currentCol));
                    } else {
                        tokens.push_back(Token(TokenType::STAR, "*", currentLine, currentCol));
                    }
                    break;
                case '/': 
                    advance();
                    if (peek() == '=') {
                        advance();
                        tokens.push_back(Token(TokenType::SLASH_EQUALS, "/=", currentLine, currentCol));
                    } else {
                        tokens.push_back(Token(TokenType::SLASH, "/", currentLine, currentCol));
                    }
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