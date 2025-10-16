#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "codegen/codegen.h"
#include "ast/ast.h"
#include "stdlib/core/stdlib_manager.h"

using namespace std;

static constexpr const char* TOOL_VERSION = "0.1.0";

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
    size_t lastslash = filename.find_last_of("/\\");
    string fname = (lastslash == string::npos) ? filename : filename.substr(lastslash + 1);
    size_t lastdot = fname.find_last_of('.');
    if (lastdot == string::npos) return fname;
    return fname.substr(0, lastdot);
}

void printHelp(const string& prog) {
    cout << "Usage: " << prog << " [options] <source.sm>\n\n";
    cout << "Options:\n";
    cout << "  -o <file>           Set output executable name (default: <source base>)\n";
    cout << "  --target <triple>   Target triple (e.g., x86_64-pc-linux-gnu, x86_64-w64-windows-gnu)\n";
    cout << "  --ir                Print generated IR to stdout\n";
    cout << "  --tokens            Print lexer tokens\n";
    cout << "  --ast               Print AST\n";
    cout << "  --emit-ir-only      Emit IR file and exit\n";
    cout << "  --keep-ir           Keep the generated IR file\n";
    cout << "  --run               Run the produced executable after successful build\n";
    cout << "  --verbose           Print extra compilation info\n";
    cout << "  --no-stdlib         Compile without linking the standard library\n";
    cout << "  --version           Print version and exit\n";
    cout << "  --help              Show this help\n";
    cout << "\nExample:\n  " << prog << " -o myprog --run hello.sm\n";
    cout << "  " << prog << " --target x86_64-pc-linux-gnu -o hello_linux hello.sm\n";
    cout << "  " << prog << " --no-stdlib -o minimal minimal.sm\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <source file> [options]\n";
        cerr << "Try '--help' for more information.\n";
        return 1;
    }

    string inputFilename;
    string outputName;
    string targetTriple;
    bool printIR = false;
    bool printTokens = false;
    bool printAST = false;
    bool emitIROnly = false;
    bool keepIR = false;
    bool runAfter = false;
    bool verbose = false;
    bool noStdlib = false;

    vector<string> args(argv + 1, argv + argc);

    for (const string& a : args) {
        if (a == "--help" || a == "-h") {
            printHelp(argv[0]);
            return 0;
        }
        if (a == "--version") {
            cout << "summit-compiler " << TOOL_VERSION << "\n";
            return 0;
        }
    }

    for (size_t i = 0; i < args.size(); ++i) {
        const string& a = args[i];

        if (a == "--ir") { printIR = true; }
        else if (a == "--tokens") { printTokens = true; }
        else if (a == "--ast") { printAST = true; }
        else if (a == "--emit-ir-only") { emitIROnly = true; }
        else if (a == "--keep-ir") { keepIR = true; }
        else if (a == "--run") { runAfter = true; }
        else if (a == "--verbose") { verbose = true; }
        else if (a == "--no-stdlib") { noStdlib = true; }
        else if (a == "-o") {
            if (i + 1 >= args.size()) { cerr << "-o expects a value\n"; return 1; }
            outputName = args[++i];
        }
        else if (a == "--target") {
            if (i + 1 >= args.size()) { cerr << "--target expects a value\n"; return 1; }
            targetTriple = args[++i];
        }
        else if (!a.empty() && a[0] == '-') {
            cerr << "Unknown option: " << a << "\n";
            cerr << "Try '--help' for a list of supported options.\n";
            return 1;
        }
        else {
            inputFilename = a;
        }
    }

    if (inputFilename.empty()) {
        cerr << "No input file provided.\n";
        cerr << "Usage: " << argv[0] << " <source file> [--help]\n";
        return 1;
    }

    try {
        string baseName = getBaseFilename(inputFilename);
        if (outputName.empty()) outputName = baseName;
        string irFilename = baseName + ".ll";

        if (verbose) {
            cerr << "Input: " << inputFilename << "\n";
            cerr << "Output: " << outputName << "\n";
            if (noStdlib) {
                cerr << "Standard library: disabled\n";
            }
        }

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

        codegen.setGlobalVariables(parser.getGlobalVariables());
        
        if (!noStdlib) {
            StdLibManager::getInstance().initializeStandardLibrary(!noStdlib);
        }
        
        ast->codegen(codegen);

        if (printIR) {
            codegen.printIR();
        }

        if (keepIR || emitIROnly) {
            codegen.printIRToFile(irFilename);
            if (verbose) cout << "IR saved to: " << irFilename << "\n";
        }

        if (emitIROnly) {
            return 0;
        }

        if (verbose) {
            cout << "Compiling to executable...\n";
            if (!targetTriple.empty()) {
                cout << "Target: " << targetTriple << "\n";
            }
        }

        if (!codegen.compileToExecutable(outputName, verbose, targetTriple, noStdlib)) {
            cerr << "Failed to compile executable.\n";
            return 1;
        }

        cout << "Compiled successfully: " << outputName << "\n";

        if (runAfter) {
#ifdef _WIN32
            string runCmd = outputName;
#else
            string runCmd = "./" + outputName;
#endif
            if (verbose) cerr << "Running: " << runCmd << "\n";
            int r = system(runCmd.c_str());
            if (verbose && r != 0) {
                cerr << "Program exited with code: " << r << "\n";
            }
        }

    } catch (const exception& e) {
        cerr << "Compilation error: " << e.what() << endl;
        return 1;
    }

    return 0;
}