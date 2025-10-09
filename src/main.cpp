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
    // Remove the redirection to see clang errors
    string command = "clang-18 -w " + irFilename + " -o " + outputFilename; // removed " >/dev/null 2>&1"
    cout << "Running: " << command << endl;
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
        cout << "Reading source file: " << inputFilename << endl;
        string source = readFile(inputFilename);
        
        cout << "Lexing..." << endl;
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        
        cout << "Found " << tokens.size() << " tokens" << endl;
        
        cout << "Parsing..." << endl;
        Parser parser(move(tokens));
        auto program = parser.parse();
        
        cout << "Parsed " << program->getStatements().size() << " statements" << endl;
        
        cout << "Generating LLVM IR..." << endl;
        CodeGen codegen;
        program->codegen(codegen);

        cout << "Writing IR to: " << irFilename << endl;
        // codegen.printIR();
        codegen.printIRToFile(irFilename);

        cout << "Compiling to executable..." << endl;
        if (compileToExecutable(irFilename, executableName)) {
            cleanupFiles(irFilename);
            cout << "Successfully created: " << executableName << endl;
        } else {
            cerr << "Compilation failed - LLVM IR generation succeeded but clang failed to create executable" << endl;
            cerr << "The intermediate file " << irFilename << " has been preserved for debugging" << endl;
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