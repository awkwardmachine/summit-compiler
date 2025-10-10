#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "codegen/codegen.h"
#include "ast/ast.h"

using namespace std;

string readFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Could not open file: " + filename);
    }
    
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

string getBaseFilename(const string& filename) {
    size_t lastdot = filename.find_last_of(".");
    if (lastdot == string::npos) return filename;
    return filename.substr(0, lastdot);
}

bool compileToExecutable(const string& irFilename, const string& outputFilename) {
    string command = "clang-18 -w " + irFilename + " -o " + outputFilename;
    int result = system(command.c_str());
    return result == 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <source file> [--ir] [--tokens] [--ast]" << endl;
        return 1;
    }
    
    string inputFilename = argv[1];
    bool printIR = false;
    bool printTokens = false;
    bool printAST = false;

    for (int i = 2; i < argc; ++i) {
        string flag = argv[i];
        if (flag == "--ir") printIR = true;
        else if (flag == "--tokens") printTokens = true;
        else if (flag == "--ast") printAST = true;
        else {
            cerr << "Unknown flag: " << flag << endl;
            return 1;
        }
    }

    string baseName = getBaseFilename(inputFilename);
    string irFilename = baseName + ".ll";
    string executableName = baseName;

    try {
        string source = readFile(inputFilename);
        
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        if (printTokens) {
            for (const auto& token : tokens) {
                cout << token.toString() << endl;
            }
        }

        Parser parser(move(tokens), source);
        auto ast = parser.parse();

        if (printAST) {
            cout << ast->toString() << endl;
        }

        CodeGen codegen;
        ast->codegen(codegen);

        if (printIR) {
            codegen.printIR();
        }

        codegen.printIR();
        codegen.printIRToFile(irFilename);

        if (!compileToExecutable(irFilename, executableName)) {
            cerr << "Failed to compile executable." << endl;
            return 1;
        }

        if (!printIR) {
            string cleanupCommand = "rm -f " + irFilename + " >/dev/null 2>&1";
            system(cleanupCommand.c_str());
        }

    } catch (const exception& e) {
        cerr << "Compilation error at stage: " << endl;
        cerr << "Error details: " << e.what() << endl;
        return 1;
    }

    return 0;
}
