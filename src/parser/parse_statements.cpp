#include "parser.h"

using namespace std;
using namespace AST;

unique_ptr<Stmt> Parser::parseEntrypointStatement() {
    advance();
    
    const Token& lastToken = tokens[current - 1];
    if (!match(TokenType::SEMICOLON)) {
        errorAt(lastToken, "Expected ';' after @entrypoint");
    }
    
    return make_unique<EntrypointStmt>();
}

unique_ptr<Stmt> Parser::parseEnumDeclaration() {
    if (!match(TokenType::ENUM)) error("Expected 'enum'");
    if (!match(TokenType::IDENTIFIER)) error("Expected enum name");
    string name = tokens[current - 1].value;
    
    registerEnumType(name);
    
    vector<pair<string, unique_ptr<Expr>>> members;
    int currentValue = 0;
    
    while (!check(TokenType::END) && !isAtEnd()) {
        if (!match(TokenType::IDENTIFIER)) error("Expected enum member name");
        string memberName = tokens[current - 1].value;
        
        unique_ptr<Expr> value = nullptr;
        if (match(TokenType::EQUALS)) {
            value = parseExpression();
        } else {
            value = make_unique<NumberExpr>(std::to_string(currentValue));
        }
        
        members.emplace_back(memberName, move(value));
        
        if (auto* numExpr = dynamic_cast<NumberExpr*>(members.back().second.get())) {
            try {
                currentValue = std::stoi(numExpr->getValue().toString()) + 1;
            } catch (...) {
                currentValue++;
            }
        } else {
            currentValue++;
        }
        
        if (!check(TokenType::END)) {
            if (!match(TokenType::COMMA)) {
                if (check(TokenType::IDENTIFIER)) {
                    continue;
                }
                break;
            }
        } else {
            break;
        }
    }
    
    if (!match(TokenType::END)) error("Expected 'end' after enum members");
    
    return make_unique<EnumDecl>(name, move(members));
}

unique_ptr<Stmt> Parser::parseVariableDeclaration() {
    bool isConst = match(TokenType::CONST);
    if (!isConst && !match(TokenType::VAR)) error("Expected 'var' or 'const'");
    if (!match(TokenType::IDENTIFIER)) error("Expected variable name");
    string name = tokens[current - 1].value;
    
    VarType type = VarType::VOID;
    
    if (check(TokenType::COLON)) {
        advance();
        type = parseType();
    } else if (!isConst) {
        error("Expected ':' and type annotation for variable '" + name + "'");
    }
    
    unique_ptr<Expr> value = nullptr;
    
    if (match(TokenType::EQUALS)) {
        value = parseExpression();
        
        if (isConst && type == VarType::VOID) {
            if (auto* moduleExpr = dynamic_cast<ModuleExpr*>(value.get())) {
                type = VarType::MODULE;
            }
            else if (auto* numberExpr = dynamic_cast<NumberExpr*>(value.get())) {
                const BigInt& bigValue = numberExpr->getValue();
                if (bigValue >= -128 && bigValue <= 127) type = VarType::INT8;
                else if (bigValue >= -32768 && bigValue <= 32767) type = VarType::INT16;
                else type = VarType::INT32;
            }
            else if (auto* stringExpr = dynamic_cast<StringExpr*>(value.get())) {
                type = VarType::STRING;
            }
            else if (auto* boolExpr = dynamic_cast<BooleanExpr*>(value.get())) {
                type = VarType::UINT0;
            }
            else if (auto* floatExpr = dynamic_cast<FloatExpr*>(value.get())) {
                type = floatExpr->getFloatType();
            }
            else {
                type = VarType::INT32;
            }
        }
    } else if (isConst) {
        error("Const variable '" + name + "' must be initialized");
    } else {
        if (type == VarType::VOID) {
            error("Variable '" + name + "' must have a type annotation when not initialized");
        }
    }
    
    const Token& lastToken = tokens[current - 1];
    if (!match(TokenType::SEMICOLON)) {
        errorAt(lastToken, "Expected ';' after variable declaration");
    }
    
    return make_unique<VariableDecl>(name, type, isConst, move(value));
}

unique_ptr<Stmt> Parser::parseAssignment() {
    if (!check(TokenType::IDENTIFIER)) error("Expected identifier for assignment");
    string name = peek().value;
    advance();
    
    if (!match(TokenType::EQUALS)) error("Expected '=' after variable name");
    auto value = parseExpression();
    
    const Token& lastToken = tokens[current - 1];
    if (!match(TokenType::SEMICOLON)) {
        errorAt(lastToken, "Expected ';' after assignment");
    }
    return make_unique<AssignmentStmt>(name, move(value));
}

unique_ptr<Stmt> Parser::parseBlock() {
    if (!match(TokenType::LBRACE)) {
        error("Expected '{' for block");
    }
    
    auto block = make_unique<BlockStmt>();
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        block->addStatement(parseStatement());
    }
    
    if (!match(TokenType::RBRACE)) {
        error("Expected '}' after block");
    }
    
    return block;
}

unique_ptr<Stmt> Parser::parseIfStatement() {
    if (!match(TokenType::IF)) {
        error("Expected 'if'");
    }
    
    if (!match(TokenType::LPAREN)) {
        error("Expected '(' after 'if'");
    }
    
    auto condition = parseExpression();
    
    if (!match(TokenType::RPAREN)) {
        error("Expected ')' after if condition");
    }
    
    if (!match(TokenType::THEN)) {
        error("Expected 'then' after if condition");
    }
    
    auto thenBlock = make_unique<BlockStmt>();
    while (!check(TokenType::ELSE) && !check(TokenType::ELSEIF) && !check(TokenType::END) && !isAtEnd()) {
        thenBlock->addStatement(parseStatement());
    }
    
    unique_ptr<Stmt> elseBranch = nullptr;
    
    if (match(TokenType::ELSEIF)) {
        if (!match(TokenType::LPAREN)) {
            error("Expected '(' after 'elseif'");
        }
        auto elseifCondition = parseExpression();
        if (!match(TokenType::RPAREN)) {
            error("Expected ')' after elseif condition");
        }
        if (!match(TokenType::THEN)) {
            error("Expected 'then' after elseif condition");
        }
        
        auto elseifThenBlock = make_unique<BlockStmt>();
        while (!check(TokenType::ELSE) && !check(TokenType::ELSEIF) && !check(TokenType::END) && !isAtEnd()) {
            elseifThenBlock->addStatement(parseStatement());
        }
        
        unique_ptr<Stmt> elseifElseBranch = nullptr;
        if (check(TokenType::ELSEIF)) {
            elseifElseBranch = parseElseIfChain();
        }
        else if (match(TokenType::ELSE)) {
            auto elseBlock = make_unique<BlockStmt>();
            while (!check(TokenType::END) && !isAtEnd()) {
                elseBlock->addStatement(parseStatement());
            }
            elseifElseBranch = move(elseBlock);
        }
        
        elseBranch = make_unique<IfStmt>(move(elseifCondition), move(elseifThenBlock), move(elseifElseBranch));
    }
    else if (match(TokenType::ELSE)) {
        auto elseBlock = make_unique<BlockStmt>();
        while (!check(TokenType::END) && !isAtEnd()) {
            elseBlock->addStatement(parseStatement());
        }
        elseBranch = move(elseBlock);
    }
    
    if (!match(TokenType::END)) {
        error("Expected 'end' after if statement");
    }
    
    return make_unique<IfStmt>(move(condition), move(thenBlock), move(elseBranch));
}

unique_ptr<Stmt> Parser::parseElseIfChain() {
    if (!match(TokenType::ELSEIF)) {
        error("Expected 'elseif'");
    }
    
    if (!match(TokenType::LPAREN)) {
        error("Expected '(' after 'elseif'");
    }
    auto condition = parseExpression();
    if (!match(TokenType::RPAREN)) {
        error("Expected ')' after elseif condition");
    }
    if (!match(TokenType::THEN)) {
        error("Expected 'then' after elseif condition");
    }
    
    auto thenBlock = make_unique<BlockStmt>();
    while (!check(TokenType::ELSE) && !check(TokenType::ELSEIF) && !check(TokenType::END) && !isAtEnd()) {
        thenBlock->addStatement(parseStatement());
    }
    
    unique_ptr<Stmt> elseBranch = nullptr;
    if (check(TokenType::ELSEIF)) {
        elseBranch = parseElseIfChain();
    }
    else if (match(TokenType::ELSE)) {
        auto elseBlock = make_unique<BlockStmt>();
        while (!check(TokenType::END) && !isAtEnd()) {
            elseBlock->addStatement(parseStatement());
        }
        elseBranch = move(elseBlock);
    }
    
    return make_unique<IfStmt>(move(condition), move(thenBlock), move(elseBranch));
}

unique_ptr<Stmt> Parser::parseFunctionDeclaration() {
    bool isEntryPoint = false;
    if (check(TokenType::ENTRYPOINT)) {
        isEntryPoint = true;
        advance();
    }
    
    if (!match(TokenType::FUNC)) error("Expected 'func'");
    if (!match(TokenType::IDENTIFIER)) error("Expected function name");
    string name = tokens[current - 1].value;
    
    if (!match(TokenType::LPAREN)) error("Expected '(' after function name");
    
    vector<pair<string, VarType>> parameters;
    if (!check(TokenType::RPAREN)) {
        do {
            if (!match(TokenType::IDENTIFIER)) error("Expected parameter name");
            string paramName = tokens[current - 1].value;
            if (!match(TokenType::COLON)) error("Expected ':' after parameter name");
            VarType paramType = parseType();
            parameters.emplace_back(paramName, paramType);
        } while (match(TokenType::COMMA));
    }
    
    if (!match(TokenType::RPAREN)) error("Expected ')' after parameters");
    
    VarType returnType = VarType::VOID;
    if (match(TokenType::MINUS)) {
        if (!match(TokenType::GREATER)) error("Expected '>' after '-' for return type");
        returnType = parseType();
    }

    auto body = make_unique<BlockStmt>();
    while (!check(TokenType::END) && !isAtEnd()) {
        body->addStatement(parseStatement());
    }
    
    if (!match(TokenType::END)) {
        error("Expected 'end' after function body");
    }
    
    return make_unique<FunctionStmt>(name, move(parameters), returnType, move(body), isEntryPoint);
}

unique_ptr<Stmt> Parser::parseReturnStatement() {
    if (!match(TokenType::RETURN)) error("Expected 'return'");
    
    unique_ptr<Expr> value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        value = parseExpression();
    }
    
    const Token& lastToken = tokens[current - 1];
    if (!match(TokenType::SEMICOLON)) {
        errorAt(lastToken, "Expected ';' after return statement");
    }
    
    return make_unique<ReturnStmt>(move(value));
}

unique_ptr<Stmt> Parser::parseWhileStatement() {
    if (!match(TokenType::WHILE)) {
        error("Expected 'while'");
    }
    
    if (!match(TokenType::LPAREN)) {
        error("Expected '(' after 'while'");
    }
    
    auto condition = parseExpression();
    
    if (!match(TokenType::RPAREN)) {
        error("Expected ')' after while condition");
    }
    
    if (!match(TokenType::THEN)) {
        error("Expected 'then' after while condition");
    }
    
    auto body = make_unique<BlockStmt>();
    while (!check(TokenType::END) && !isAtEnd()) {
        body->addStatement(parseStatement());
    }
    
    if (!match(TokenType::END)) {
        error("Expected 'end' after while statement");
    }
    
    return make_unique<WhileStmt>(move(condition), move(body));
}

unique_ptr<Stmt> Parser::parseForLoopStatement() {
    if (!match(TokenType::FOR)) {
        error("Expected 'for'");
    }

    if (!match(TokenType::LPAREN)) {
        error("Expected '(' after 'for'");
    }

    if (!match(TokenType::IDENTIFIER)) {
        error("Expected variable name in for loop");
    }
    string varName = tokens[current - 1].value;

    if (!match(TokenType::COLON)) {
        error("Expected ':' after variable name in for loop");
    }
    VarType varType = parseType();

    unique_ptr<Expr> initializer = nullptr;
    if (match(TokenType::EQUALS)) {
        initializer = parseExpression();
    }

    if (!match(TokenType::SEMICOLON)) {
        error("Expected ';' after for loop initializer");
    }

    auto condition = parseExpression();
    if (!match(TokenType::SEMICOLON)) {
        error("Expected ';' after for loop condition");
    }

    unique_ptr<Expr> increment = nullptr;

    if (check(TokenType::IDENTIFIER)) {
        string incVarName = peek().value;
        advance();

        if (check(TokenType::INCREMENT) || check(TokenType::DECREMENT)) {
            bool isIncrement = (peek().type == TokenType::INCREMENT);
            advance();
            auto varExpr = make_unique<VariableExpr>(incVarName);
            auto one = make_unique<NumberExpr>("1");
            BinaryOp op = isIncrement ? BinaryOp::ADD : BinaryOp::SUBTRACT;
            increment = make_unique<BinaryExpr>(op, move(varExpr), move(one));
        }
        else if (check(TokenType::PLUS_EQUALS) || check(TokenType::MINUS_EQUALS) ||
                 check(TokenType::STAR_EQUALS) || check(TokenType::SLASH_EQUALS)) {
            TokenType opToken = peek().type;
            advance();
            auto right = parseExpression();

            BinaryOp binOp;
            switch (opToken) {
                case TokenType::PLUS_EQUALS: binOp = BinaryOp::ADD; break;
                case TokenType::MINUS_EQUALS: binOp = BinaryOp::SUBTRACT; break;
                case TokenType::STAR_EQUALS: binOp = BinaryOp::MULTIPLY; break;
                case TokenType::SLASH_EQUALS: binOp = BinaryOp::DIVIDE; break;
                default: throw runtime_error("Unknown compound assignment operator");
            }

            auto leftVar = make_unique<VariableExpr>(incVarName);
            increment = make_unique<BinaryExpr>(binOp, move(leftVar), move(right));
        }
        else {
            current--;
            increment = parseExpression();
        }
    } else {
        increment = parseExpression();
    }

    if (!match(TokenType::RPAREN)) {
        error("Expected ')' after for loop header");
    }

    if (!match(TokenType::DO)) {
        error("Expected 'do' after for loop header");
    }

    auto body = make_unique<BlockStmt>();
    while (!check(TokenType::END) && !isAtEnd()) {
        body->addStatement(parseStatement());
    }

    if (!match(TokenType::END)) {
        error("Expected 'end' after for loop body");
    }

    return make_unique<ForLoopStmt>(varName, varType, move(initializer),
                                    move(condition), move(increment), move(body));
}

unique_ptr<Stmt> Parser::parseStatement() {
    if (check(TokenType::ENTRYPOINT)) {
        return parseEntrypointStatement();
    }
    
    if (check(TokenType::FUNC)) {
        return parseFunctionDeclaration();
    }
    if (check(TokenType::RETURN)) {
        return parseReturnStatement();
    }
    if (check(TokenType::IF)) {
        return parseIfStatement();
    }
    if (check(TokenType::WHILE)) {
        return parseWhileStatement();
    }
    if (check(TokenType::FOR)) {
        return parseForLoopStatement();
    }
    if (check(TokenType::ENUM)) {
        return parseEnumDeclaration();
    }
    if (check(TokenType::VAR) || check(TokenType::CONST)) {
        return parseVariableDeclaration();
    }
    
    if (check(TokenType::IDENTIFIER)) {
        return parseAssignmentOrIncrement();
    }
    
    auto expr = parseExpression();
    
    const Token& lastToken = tokens[current - 1];
    if (!match(TokenType::SEMICOLON)) {
        errorAt(lastToken, "Expected ';' after expression");
    }
    return make_unique<ExprStmt>(move(expr));
}

unique_ptr<Stmt> Parser::parseAssignmentOrIncrement() {
    string name = peek().value;
    advance();
    
    if (check(TokenType::INCREMENT) || check(TokenType::DECREMENT)) {
        bool isIncrement = (peek().type == TokenType::INCREMENT);
        advance();
        
        if (!match(TokenType::SEMICOLON)) {
            error("Expected ';' after " + string(isIncrement ? "++" : "--"));
        }
        
        auto varExpr = make_unique<VariableExpr>(name);
        auto one = make_unique<NumberExpr>("1");
        BinaryOp op = isIncrement ? BinaryOp::ADD : BinaryOp::SUBTRACT;
        auto binExpr = make_unique<BinaryExpr>(op, move(varExpr), move(one));
        return make_unique<AssignmentStmt>(name, move(binExpr));
    }
    
    if (check(TokenType::PLUS_EQUALS) || check(TokenType::MINUS_EQUALS) ||
        check(TokenType::STAR_EQUALS) || check(TokenType::SLASH_EQUALS)) {
        
        TokenType opToken = peek().type;
        advance();
        
        auto right = parseExpression();

        if (!match(TokenType::SEMICOLON)) {
            error("Expected ';' after compound assignment");
        }

        BinaryOp binOp;
        switch (opToken) {
            case TokenType::PLUS_EQUALS: binOp = BinaryOp::ADD; break;
            case TokenType::MINUS_EQUALS: binOp = BinaryOp::SUBTRACT; break;
            case TokenType::STAR_EQUALS: binOp = BinaryOp::MULTIPLY; break;
            case TokenType::SLASH_EQUALS: binOp = BinaryOp::DIVIDE; break;
            default: throw runtime_error("Unknown compound assignment operator");
        }
        
        auto leftVar = make_unique<VariableExpr>(name);
        auto binExpr = make_unique<BinaryExpr>(binOp, move(leftVar), move(right));
        return make_unique<AssignmentStmt>(name, move(binExpr));
    }
    
    if (check(TokenType::EQUALS)) {
        return parseAssignment();
    }
    
    current--;
    auto expr = parseExpression();
    
    const Token& lastToken = tokens[current - 1];
    if (!match(TokenType::SEMICOLON)) {
        errorAt(lastToken, "Expected ';' after expression");
    }
    return make_unique<ExprStmt>(move(expr));
}