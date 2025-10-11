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


unique_ptr<Stmt> Parser::parseVariableDeclaration() {
    bool isConst = match(TokenType::CONST);
    if (!isConst && !match(TokenType::VAR)) error("Expected 'var' or 'const'");
    if (!match(TokenType::IDENTIFIER)) error("Expected variable name");
    string name = tokens[current - 1].value;
    if (!match(TokenType::COLON)) error("Expected ':' after variable name");
    VarType type = parseType();
    unique_ptr<Expr> value = nullptr;
    if (match(TokenType::EQUALS)) value = parseExpression();
    
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
    
    // Parse function body statements until 'end'
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

unique_ptr<Stmt> Parser::parseStatement() {
    // Check for @entrypoint; statement first
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
    if (check(TokenType::VAR) || check(TokenType::CONST)) {
        return parseVariableDeclaration();
    }
    if (check(TokenType::IDENTIFIER) && checkNext(TokenType::EQUALS)) { 
        return parseAssignment();
    }
    
    auto expr = parseExpression();
    
    const Token& lastToken = tokens[current - 1];
    if (!match(TokenType::SEMICOLON)) {
        errorAt(lastToken, "Expected ';' after expression");
    }
    return make_unique<ExprStmt>(move(expr));
}