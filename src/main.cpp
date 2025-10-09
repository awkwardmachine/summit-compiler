#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "codegen/codegen.h"

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

void cleanupFiles(const string& irFilename) {
    string cleanupCommand = "rm -f " + irFilename + " >/dev/null 2>&1";
    system(cleanupCommand.c_str());
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <source file>" << endl;
        return 1;
    }
    
    string inputFilename = argv[1];
    string baseName = getBaseFilename(inputFilename);
    string irFilename = baseName + ".ll";
    string executableName = baseName;
    
    try {
        string source = readFile(inputFilename);
        
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        
        Parser parser(move(tokens), source);
        auto ast = parser.parse();

        CodeGen codegen;
        ast->codegen(codegen);

        // codegen.printIR();
        codegen.printIRToFile(irFilename);

        if (compileToExecutable(irFilename, executableName)) {
            cleanupFiles(irFilename);
        } else {
            return 1;
        }
        
    } catch (const exception& e) {
        cerr << "Compilation error at stage: " << endl;
        cerr << "Error details: " << e.what() << endl;
        cleanupFiles(irFilename);
        return 1;
    }
    
    return 0;
}