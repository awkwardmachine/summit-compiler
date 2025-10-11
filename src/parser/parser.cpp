#include "parser.h"

using namespace std;
using namespace AST;

unique_ptr<Program> Parser::parse() {
    auto program = make_unique<Program>();
    bool nextFunctionIsEntryPoint = false;
    
    while (!isAtEnd()) {
        if (check(TokenType::ENTRYPOINT)) {
            if (program->getHasEntryPoint()) {
                error("Only one @entrypoint allowed per program");
            }
            
            auto entrypointStmt = parseEntrypointStatement();
            nextFunctionIsEntryPoint = true;
            continue;
        }

        auto stmt = parseStatement();
        
        if (nextFunctionIsEntryPoint) {
            if (auto* funcStmt = dynamic_cast<FunctionStmt*>(stmt.get())) {
                program->setEntryPointFunction(funcStmt->getName());
                std::cout << "DEBUG: Marking function '" << funcStmt->getName() 
                          << "' as entry point" << std::endl;
            } else {
                error("@entrypoint must be followed by a function declaration");
            }
            nextFunctionIsEntryPoint = false;
        }
        
        program->addStatement(move(stmt));
    }
    
    return program;
}