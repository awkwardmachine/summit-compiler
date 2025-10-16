#include "parser.h"
#include <stdexcept>

using namespace std;
using namespace AST;

unique_ptr<Expr> Parser::parseExpression() {
    return parseCastExpression();
}

unique_ptr<Expr> Parser::parseCastExpression() {
    auto expr = parseBinaryExpression(0);
    if (match(TokenType::AS)) {
        VarType targetType = parseType();
        return make_unique<CastExpr>(move(expr), targetType);
    }
    return expr;
}

unique_ptr<Expr> Parser::parsePrimary() {
    const Token& tok = peek();

    if (check(TokenType::ENTRYPOINT)) {
        error("@entrypoint should be used as a standalone statement, not in an expression");
    }

    if (check(TokenType::IDENTIFIER) && checkNext(TokenType::LBRACE)) {
        return parseStructLiteral();
    }

    if (check(TokenType::BUILTIN) || (check(TokenType::IDENTIFIER) && peek().value == "@get")) {
        return parseBuiltinCall();
    }

    if (match(TokenType::NUMBER)) 
        return make_unique<NumberExpr>(tokens[current - 1].value);

    if (match(TokenType::FLOAT_LITERAL)) {
        string floatStr = tokens[current - 1].value;
        try {
            double value = stod(floatStr);
            return make_unique<FloatExpr>(value, VarType::FLOAT64);
        } catch (const exception&) {
            error("Invalid float literal: " + floatStr);
        }
    }

    if (match(TokenType::STRING_LITERAL)) {
        return make_unique<StringExpr>(tokens[current - 1].value);
    }

    if (match(TokenType::BACKTICK_STRING)) {
        string formatStr = tokens[current - 1].value;
        vector<unique_ptr<Expr>> expressions = extractExpressionsFromFormat(formatStr);
        return make_unique<FormatStringExpr>(formatStr, move(expressions));
    }

    if (match(TokenType::TRUE)) return make_unique<BooleanExpr>(true);
    if (match(TokenType::FALSE)) return make_unique<BooleanExpr>(false);

    if (match(TokenType::BUILTIN)) {
        string name;
        string builtinToken = tokens[current - 1].value;
        if (builtinToken.length() > 1 && builtinToken[0] == '@') {
            name = builtinToken.substr(1);
        } else {
            if (!check(TokenType::IDENTIFIER)) error("Expected identifier after '@'");
            name = advance().value;
        }

        if (!match(TokenType::LPAREN)) error("Expected '(' after @" + name);
        vector<unique_ptr<Expr>> args;
        if (!check(TokenType::RPAREN)) {
            do { args.push_back(parseExpression()); } while(match(TokenType::COMMA));
        }
        if (!match(TokenType::RPAREN)) error("Expected ')' after arguments");
        return make_unique<CallExpr>("@" + name, move(args));
    }

    if (match(TokenType::IDENTIFIER)) {
        string name = tokens[current - 1].value;
        
        if (match(TokenType::DOT)) {
            auto object = make_unique<VariableExpr>(name);
            auto memberAccess = parseMemberAccess(move(object));
            
            if (check(TokenType::LPAREN)) {
                advance();
                vector<unique_ptr<Expr>> args;
                if (!check(TokenType::RPAREN)) {
                    do { args.push_back(parseExpression()); } while(match(TokenType::COMMA));
                }
                if (!match(TokenType::RPAREN)) error("Expected ')' after function arguments");
                return make_unique<CallExpr>(move(memberAccess), move(args));
            }
            
            return memberAccess;
        }
        
        if (match(TokenType::LPAREN)) {
            vector<unique_ptr<Expr>> args;
            if (!check(TokenType::RPAREN)) {
                do { args.push_back(parseExpression()); } while(match(TokenType::COMMA));
            }
            if (!match(TokenType::RPAREN)) error("Expected ')' after function arguments");
            return make_unique<CallExpr>(name, move(args));
        }
        
        return make_unique<VariableExpr>(name);
    }

    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        if (!match(TokenType::RPAREN)) error("Expected ')' after expression");
        return expr;
    }

    error("Expected expression, but found: " + tok.value);
    return nullptr;
}

unique_ptr<Expr> Parser::parseMemberAccess(unique_ptr<Expr> object) {
    if (!match(TokenType::IDENTIFIER)) {
        error("Expected identifier after '.'");
    }
    string member = tokens[current - 1].value;

    if (auto* varExpr = dynamic_cast<VariableExpr*>(object.get())) {
        std::string varName = varExpr->getName();
        
        if (isEnumType(varName)) {
            return make_unique<EnumValueExpr>(varName, member);
        }
        
        if (!varName.empty() && isupper(varName[0])) {
            return make_unique<EnumValueExpr>(varName, member);
        }
    }
    auto memberAccess = make_unique<MemberAccessExpr>(move(object), member);

    if (match(TokenType::LPAREN)) {
        vector<unique_ptr<Expr>> args;
        if (!check(TokenType::RPAREN)) {
            do { 
                args.push_back(parseExpression()); 
            } while (match(TokenType::COMMA));
        }
        if (!match(TokenType::RPAREN)) {
            error("Expected ')' after function arguments");
        }
        return make_unique<CallExpr>(move(memberAccess), move(args));
    }

    if (match(TokenType::DOT)) {
        return parseMemberAccess(move(memberAccess));
    }
    
    return memberAccess;
}

unique_ptr<Expr> Parser::parseUnaryExpression() {
    if (match(TokenType::NOT) || match(TokenType::EXCLAMATION)) {
        auto operand = parseUnaryExpression();
        return make_unique<UnaryExpr>(UnaryOp::LOGICAL_NOT, move(operand));
    }
    
    if (match(TokenType::MINUS)) {
        auto operand = parseUnaryExpression();
        return make_unique<UnaryExpr>(UnaryOp::NEGATE, move(operand));
    }

    if (match(TokenType::TILDE)) {
        auto operand = parseUnaryExpression();
        return make_unique<UnaryExpr>(UnaryOp::BITWISE_NOT, move(operand));
    }
    
    return parsePrimary();
}

unique_ptr<Expr> Parser::parseBinaryExpression(int minPrecedence) {
    auto left = parseUnaryExpression();

    while (true) {
        BinaryOp op;
        int precedence = 0;

        if (check(TokenType::OR) || check(TokenType::OR_OR)) {
            op = BinaryOp::LOGICAL_OR;
            precedence = 1;
        }
        else if (check(TokenType::AND) || check(TokenType::AND_AND)) {
            op = BinaryOp::LOGICAL_AND;
            precedence = 2;
        }
        else if (check(TokenType::PIPE)) {
            op = BinaryOp::BITWISE_OR;
            precedence = 3;
        }
        else if (check(TokenType::CARET)) {
            op = BinaryOp::BITWISE_XOR;
            precedence = 4;
        }
        else if (check(TokenType::AMPERSAND)) {
            op = BinaryOp::BITWISE_AND;
            precedence = 5;
        }
        else if (check(TokenType::EQUAL_EQUAL)) {
            op = BinaryOp::EQUAL;
            precedence = 6;
        }
        else if (check(TokenType::NOT_EQUAL)) {
            op = BinaryOp::NOT_EQUAL;
            precedence = 6;
        }
        else if (check(TokenType::GREATER)) {
            op = BinaryOp::GREATER;
            precedence = 7;
        }
        else if (check(TokenType::LESS)) {
            op = BinaryOp::LESS;
            precedence = 7;
        }
        else if (check(TokenType::GREATER_EQUAL)) {
            op = BinaryOp::GREATER_EQUAL;
            precedence = 7;
        }
        else if (check(TokenType::LESS_EQUAL)) {
            op = BinaryOp::LESS_EQUAL;
            precedence = 7;
        }
        else if (check(TokenType::LEFT_SHIFT)) {
            op = BinaryOp::LEFT_SHIFT;
            precedence = 8;
        }
        else if (check(TokenType::RIGHT_SHIFT)) {
            op = BinaryOp::RIGHT_SHIFT;
            precedence = 8;
        }
        else if (check(TokenType::PLUS)) {
            op = BinaryOp::ADD;
            precedence = 9;
        }
        else if (check(TokenType::MINUS)) {
            op = BinaryOp::SUBTRACT;
            precedence = 9;
        }
        else if (check(TokenType::STAR)) {
            op = BinaryOp::MULTIPLY;
            precedence = 10;
        }
        else if (check(TokenType::SLASH)) {
            op = BinaryOp::DIVIDE;
            precedence = 10;
        }
        else if (check(TokenType::PERCENT)) {
            op = BinaryOp::MODULUS;
            precedence = 10;
        }
        else break;

        if (precedence < minPrecedence) break;
        
        advance();
        auto right = parseBinaryExpression(precedence + 1);
        left = make_unique<BinaryExpr>(op, move(left), move(right));
    }
    return left;
}

unique_ptr<Expr> Parser::parseStructLiteral() {
    if (!match(TokenType::IDENTIFIER)) error("Expected struct name");
    string structName = tokens[current - 1].value;
    
    if (!match(TokenType::LBRACE)) {
        error("Expected '{' for struct literal");
    }
    
    vector<pair<string, unique_ptr<Expr>>> fields;
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        if (!match(TokenType::IDENTIFIER)) error("Expected field name");
        string fieldName = tokens[current - 1].value;
        
        if (!match(TokenType::COLON)) {
            error("Expected ':' after field name");
        }
        
        auto value = parseExpression();
        fields.emplace_back(fieldName, move(value));
        
        if (!check(TokenType::RBRACE)) {
            if (!match(TokenType::COMMA)) {
                error("Expected ',' or '}' after field value");
            }
        }
    }
    
    if (!match(TokenType::RBRACE)) {
        error("Expected '}' after struct literal");
    }
    
    return make_unique<StructLiteralExpr>(structName, move(fields));
}

unique_ptr<Expr> Parser::parseBuiltinCall() {
    if (!match(TokenType::BUILTIN)) error("Expected '@' for builtin call");
    
    string builtinToken = tokens[current - 1].value;
    string funcName;
    
    if (builtinToken.length() > 1 && builtinToken[0] == '@') {
        funcName = builtinToken.substr(1);
    } else {
        if (!check(TokenType::IDENTIFIER)) error("Expected builtin function name after '@'");
        funcName = advance().value;
    }
    
    if (funcName == "import") {
        if (!match(TokenType::LPAREN)) error("Expected '(' after @import");
        
        if (!match(TokenType::STRING_LITERAL)) error("Expected string literal for module name");
        string moduleName = tokens[current - 1].value;
        
        if (!match(TokenType::RPAREN)) error("Expected ')' after module name");
        
        return make_unique<ModuleExpr>(moduleName);
    }
    
    if (!match(TokenType::LPAREN)) error("Expected '(' after @" + funcName);
    vector<unique_ptr<Expr>> args;
    if (!check(TokenType::RPAREN)) {
        do { args.push_back(parseExpression()); } while(match(TokenType::COMMA));
    }
    if (!match(TokenType::RPAREN)) error("Expected ')' after arguments");
    
    return make_unique<CallExpr>("@" + funcName, move(args));
}
