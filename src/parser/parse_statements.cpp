#include "parser.h"
#include <vector>
#include "ast_types.h"

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
    
    if (isInGlobalScope()) {
        registerGlobalVariable(name);
        std::cout << "DEBUG: Registered global variable: " << name << std::endl;
    }
    
    VarType type = VarType::VOID;
    std::string structName = "";

    if (check(TokenType::COLON)) {
        advance();
        if (check(TokenType::IDENTIFIER)) {
            string typeName = peek().value;
            advance();
            
            if (isEnumType(typeName)) {
                type = VarType::INT32;
            } else if (isStructType(typeName)) {
                type = VarType::STRUCT;
                structName = typeName;
            } else {
                throw std::runtime_error("Unknown type: " + typeName);
            }
        } else {
            type = parseType();
        }
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
    
    std::cout << "DEBUG: Creating VariableDecl for '" << name << "' with type " << static_cast<int>(type) << " and structName '" << structName << "'" << std::endl;
    return make_unique<VariableDecl>(name, type, isConst, move(value), structName);
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
    
    enterScope();
    
    auto block = make_unique<BlockStmt>();
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        block->addStatement(parseStatement());
    }
    
    if (!match(TokenType::RBRACE)) {
        error("Expected '}' after block");
    }

    exitScope();
    
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
    
    enterScope();
    
    auto thenBlock = make_unique<BlockStmt>();
    while (!check(TokenType::ELSE) && !check(TokenType::ELSEIF) && !check(TokenType::END) && !isAtEnd()) {
        thenBlock->addStatement(parseStatement());
    }
    
    exitScope();
    
    unique_ptr<Stmt> elseBranch = nullptr;
    
    if (match(TokenType::ELSEIF)) {
        enterScope();
        
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
        
        exitScope();
        
        unique_ptr<Stmt> elseifElseBranch = nullptr;
        if (check(TokenType::ELSEIF)) {
            elseifElseBranch = parseElseIfChain();
        }
        else if (match(TokenType::ELSE)) {
            enterScope();
            
            auto elseBlock = make_unique<BlockStmt>();
            while (!check(TokenType::END) && !isAtEnd()) {
                elseBlock->addStatement(parseStatement());
            }
            exitScope();
            
            elseifElseBranch = move(elseBlock);
        }
        
        elseBranch = make_unique<IfStmt>(move(elseifCondition), move(elseifThenBlock), move(elseifElseBranch));
    }
    else if (match(TokenType::ELSE)) {
        enterScope();
        
        auto elseBlock = make_unique<BlockStmt>();
        while (!check(TokenType::END) && !isAtEnd()) {
            elseBlock->addStatement(parseStatement());
        }
        exitScope();
        
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
    
    enterScope();
    
    if (!match(TokenType::LPAREN)) error("Expected '(' after function name");
    
    vector<pair<string, VarType>> parameters;
    if (!check(TokenType::RPAREN)) {
        do {
            if (!match(TokenType::IDENTIFIER)) error("Expected parameter name");
            string paramName = tokens[current - 1].value;
            if (!match(TokenType::COLON)) error("Expected ':' after parameter name");
            
            VarType paramType;

            if (check(TokenType::IDENTIFIER)) {
                string typeName = peek().value;
                
                if (isEnumType(typeName)) {
                    paramType = VarType::INT32;
                    advance();
                } else if (isStructType(typeName)) {
                    paramType = VarType::STRUCT;
                    advance();
                } else {
                    paramType = parseType();
                }
            } else {
                paramType = parseType();
            }
            
            parameters.emplace_back(paramName, paramType);
        } while (match(TokenType::COMMA));
    }
    
    if (!match(TokenType::RPAREN)) error("Expected ')' after parameters");
    
    VarType returnType = VarType::VOID;
    std::string returnStructName = "";
    
    if (match(TokenType::MINUS)) {
        if (!match(TokenType::GREATER)) error("Expected '>' after '-' for return type");
        
        if (check(TokenType::IDENTIFIER)) {
            string typeName = peek().value;
            
            if (isEnumType(typeName)) {
                returnType = VarType::INT32;
                advance();
            } else if (isStructType(typeName)) {
                returnType = VarType::STRUCT;
                returnStructName = typeName;
                advance();
            } else {
                returnType = parseType();
            }
        } else {
            returnType = parseType();
        }
    }

    auto body = make_unique<BlockStmt>();
    while (!check(TokenType::END) && !isAtEnd()) {
        body->addStatement(parseStatement());
    }
    
    if (!match(TokenType::END)) {
        error("Expected 'end' after function body");
    }
    
    exitScope();
    
    return make_unique<FunctionStmt>(name, move(parameters), returnType, move(body), isEntryPoint, returnStructName);
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

    enterScope();

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

    exitScope();

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
unique_ptr<Stmt> Parser::parseStructDeclaration() {
    if (!match(TokenType::STRUCT)) error("Expected 'struct'");
    if (!match(TokenType::IDENTIFIER)) error("Expected struct name");
    string name = tokens[current - 1].value;

    registerStructType(name);
    
    vector<pair<string, VarType>> fields;
    vector<unique_ptr<FunctionStmt>> methods;
    
    unordered_map<string, unique_ptr<Expr>> fieldDefaults;
    
    while (!check(TokenType::END) && !isAtEnd()) {
        if (check(TokenType::IDENTIFIER) && checkNext(TokenType::COLON)) {
            string fieldName = peek().value;
            advance();
            advance();
            
            VarType fieldType;

            if (check(TokenType::IDENTIFIER)) {
                string typeName = peek().value;
                
                if (isEnumType(typeName)) {
                    fieldType = VarType::INT32;
                    advance();
                } else if (isStructType(typeName)) {
                    fieldType = VarType::STRUCT;
                    advance();
                } else {
                    fieldType = parseType();
                }
            } else {
                fieldType = parseType();
            }
            
            fields.emplace_back(fieldName, fieldType);

            if (match(TokenType::EQUALS)) {
                auto defaultValue = parseExpression();
                fieldDefaults[fieldName] = std::move(defaultValue);
            }
            
            if (!match(TokenType::SEMICOLON)) {
                error("Expected ';' after field declaration");
            }
        }
        else if (check(TokenType::FUNC)) {
            auto method = parseMethodDeclaration(name);
            methods.push_back(move(method));
        }
        else {
            error("Expected field declaration or method in struct");
        }
    }
    
    if (!match(TokenType::END)) {
        error("Expected 'end' after struct definition");
    }
    
    auto structDecl = make_unique<StructDecl>(name, move(fields), move(methods));
    
    for (auto& [fieldName, defaultValue] : fieldDefaults) {
        structDecl->addFieldDefault(fieldName, std::move(defaultValue));
    }
    
    return structDecl;
}
unique_ptr<FunctionStmt> Parser::parseMethodDeclaration(const string& structName) {
    if (!match(TokenType::FUNC)) error("Expected 'func'");
    if (!match(TokenType::IDENTIFIER)) error("Expected method name");
    string methodName = tokens[current - 1].value;

    enterScope();
    
    if (!match(TokenType::LPAREN)) error("Expected '(' after method name");
    
    vector<pair<string, VarType>> parameters;
    
    parameters.emplace_back("self", VarType::STRUCT);
    
    if (!check(TokenType::RPAREN)) {
        do {
            if (!match(TokenType::IDENTIFIER)) error("Expected parameter name");
            string paramName = tokens[current - 1].value;
            if (!match(TokenType::COLON)) error("Expected ':' after parameter name");
            
            VarType paramType;
            
            if (check(TokenType::IDENTIFIER)) {
                string typeName = peek().value;
                
                if (isEnumType(typeName)) {
                    paramType = VarType::INT32;
                    advance();
                } else if (isStructType(typeName)) {
                    paramType = VarType::STRUCT;
                    advance();
                } else {
                    paramType = parseType();
                }
            } else {
                paramType = parseType();
            }
            
            parameters.emplace_back(paramName, paramType);
        } while (match(TokenType::COMMA));
    }
    
    if (!match(TokenType::RPAREN)) error("Expected ')' after parameters");
    
    VarType returnType = VarType::VOID;
    std::string returnStructName = "";
    
    if (match(TokenType::MINUS)) {
        if (!match(TokenType::GREATER)) error("Expected '>' after '-' for return type");
        
        if (check(TokenType::IDENTIFIER)) {
            string typeName = peek().value;
            std::cout << "DEBUG parseMethodDeclaration: Found return type identifier: '" << typeName << "'" << std::endl;
            
            if (isEnumType(typeName)) {
                returnType = VarType::INT32;
                advance();
                std::cout << "DEBUG parseMethodDeclaration: Set return type to INT32 (enum)" << std::endl;
            } else if (isStructType(typeName)) {
                returnType = VarType::STRUCT;
                returnStructName = typeName;
                advance();
                std::cout << "DEBUG parseMethodDeclaration: Set return type to STRUCT with name: '" << returnStructName << "'" << std::endl;
            } else {
                returnType = parseType();
                std::cout << "DEBUG parseMethodDeclaration: Set return type to: " << static_cast<int>(returnType) << std::endl;
            }
        } else {
            returnType = parseType();
            std::cout << "DEBUG parseMethodDeclaration: Set return type to: " << static_cast<int>(returnType) << std::endl;
        }
    } else {
        std::cout << "DEBUG parseMethodDeclaration: No return type specified, defaulting to VOID" << std::endl;
    }

    auto body = make_unique<BlockStmt>();
    while (!check(TokenType::END) && !isAtEnd()) {
        body->addStatement(parseStatement());
    }
    
    if (!match(TokenType::END)) {
        error("Expected 'end' after method body");
    }
    
    exitScope();

    std::string fullMethodName = structName + "." + methodName;
    
    std::cout << "DEBUG parseMethodDeclaration: Creating method '" << fullMethodName 
              << "' with return type " << static_cast<int>(returnType)
              << " and return struct name '" << returnStructName << "'" << std::endl;
    
    auto method = make_unique<FunctionStmt>(fullMethodName, std::move(parameters), returnType, std::move(body), false, returnStructName);
    
    return method;
}

unique_ptr<Stmt> Parser::parseStatement() {
    std::cout << "DEBUG parseStatement: current token = " 
              << tokens[current].value 
              << " type = " << static_cast<int>(tokens[current].type)
              << " at line " << tokens[current].line << std::endl;
    if (check(TokenType::ENTRYPOINT)) {
        return parseEntrypointStatement();
    }
    
    if (check(TokenType::FUNC)) {
        return parseFunctionDeclaration();
    }
    if (check(TokenType::STRUCT)) {
        return parseStructDeclaration();
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
    
    if (check(TokenType::STOP)) {
        return parseBreakStatement();
    }
    if (check(TokenType::NEXT)) {
        return parseContinueStatement();
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
    std::cout << "DEBUG parseAssignmentOrIncrement: current token = " 
              << tokens[current].value << " at line " 
              << tokens[current].line << std::endl;
    size_t savedPos = current;
    
    if (check(TokenType::IDENTIFIER)) {
        string firstName = peek().value;
        advance();
        
        if (check(TokenType::DOT)) {
            advance();
            
            if (!check(TokenType::IDENTIFIER)) {
                error("Expected member name after '.'");
            }
            string memberName = peek().value;
            advance();
            
            if (check(TokenType::EQUALS)) {
                advance();
                
                auto value = parseExpression();
                
                const Token& lastToken = tokens[current - 1];
                if (!match(TokenType::SEMICOLON)) {
                    errorAt(lastToken, "Expected ';' after member assignment");
                }
                
                return make_unique<MemberAssignmentStmt>(
                    make_unique<VariableExpr>(firstName),
                    memberName,
                    move(value)
                );
            }
            
            current = savedPos;
            auto expr = parseExpression();
            
            const Token& lastToken = tokens[current - 1];
            if (!match(TokenType::SEMICOLON)) {
                errorAt(lastToken, "Expected ';' after expression");
            }
            return make_unique<ExprStmt>(move(expr));
        }

        current = savedPos;
    }
    
    string name = peek().value;
    
    if (checkNext(TokenType::INCREMENT) || checkNext(TokenType::DECREMENT)) {
        advance();
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
    
    if (checkNext(TokenType::PLUS_EQUALS) || checkNext(TokenType::MINUS_EQUALS) ||
        checkNext(TokenType::STAR_EQUALS) || checkNext(TokenType::SLASH_EQUALS)) {
        
        advance();
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

    if (checkNext(TokenType::EQUALS)) {
        advance();
        advance();
        auto value = parseExpression();
        
        const Token& lastToken = tokens[current - 1];
        if (!match(TokenType::SEMICOLON)) {
            errorAt(lastToken, "Expected ';' after assignment");
        }
        return make_unique<AssignmentStmt>(name, move(value));
    }

    auto expr = parseExpression();
    
    const Token& lastToken = tokens[current - 1];
    if (!match(TokenType::SEMICOLON)) {
        errorAt(lastToken, "Expected ';' after expression");
    }
    return make_unique<ExprStmt>(move(expr));
}

unique_ptr<Stmt> Parser::parseBreakStatement() {
    if (!match(TokenType::STOP)) {
        error("Expected 'STOP' for break statement");
    }
    
    const Token& lastToken = tokens[current - 1];
    if (!match(TokenType::SEMICOLON)) {
        errorAt(lastToken, "Expected ';' after STOP");
    }
    
    return make_unique<BreakStmt>();
}

unique_ptr<Stmt> Parser::parseContinueStatement() {
    if (!match(TokenType::NEXT)) {
        error("Expected 'NEXT' for continue statement");
    }
    
    const Token& lastToken = tokens[current - 1];
    if (!match(TokenType::SEMICOLON)) {
        errorAt(lastToken, "Expected ';' after NEXT");
    }
    
    return make_unique<ContinueStmt>();
}