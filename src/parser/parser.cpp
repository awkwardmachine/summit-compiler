#include "parser.h"

using namespace std;
using namespace AST;

unique_ptr<Program> Parser::parse() {
    auto program = make_unique<Program>();
    while (!isAtEnd()) program->addStatement(parseStatement());
    return program;
}