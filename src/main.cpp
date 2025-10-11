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
#include "codegen/stdlib.h"

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

string escapeShellArg(const string& s) {
    string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\"'\"'";
        else out += c;
    }
    out += "'";
    return out;
}

bool runCommand(const string& cmd, bool verbose) {
    if (verbose) {
        cerr << "[cmd] " << cmd << endl;
    }
    int r = system(cmd.c_str());
    return r == 0;
}

void printHelp(const string& prog) {
    cout << "Usage: " << prog << " [options] <source.sm>\n\n";
    cout << "Options:\n";
    cout << "  -o <file>           Set output executable name (default: <source base>)\n";
    cout << "  --ir                Print generated IR to stdout\n";
    cout << "  --tokens            Print lexer tokens\n";
    cout << "  --ast               Print AST\n";
    cout << "  --emit-ir-only      Emit IR file and do not run the system compiler/linker\n";
    cout << "  --keep-ir           Keep the generated IR file (don't delete it)\n";
    cout << "  -O0|-O1|-O2|-O3    Optimization level passed to system compiler (default: -O0)\n";
    cout << "  --compiler <path>   Path to system C compiler (default: clang-18)\n";
    cout << "  --ldflags '<flags>' Additional linker flags to pass\n";
    cout << "  --run               Run the produced executable after successful build\n";
    cout << "  --verbose           Print commands and extra info\n";
    cout << "  --version           Print version and exit\n";
    cout << "  --help              Show this help\n";
    cout << "\nExample:\n  " << prog << " -O2 -o myprog --run --keep-ir hello.sm\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <source file> [options]\n";
        cerr << "Try '--help' for more information.\n";
        return 1;
    }

    string inputFilename;
    string outputName;
    bool printIR = false;
    bool printTokens = false;
    bool printAST = false;
    bool emitIROnly = false;
    bool keepIR = false;
    bool runAfter = false;
    bool verbose = false;
    string compilerPath = "clang-18";
    string ldflags;
    string optLevel = "-O0";

    vector<string> args(argv + 1, argv + argc);

    for (const string& a : args) {
        if (a == "--help" || a == "-h") {
            printHelp(argv[0]);
            return 0;
        }
        if (a == "--version") {
            cout << "sm-compiler " << TOOL_VERSION << "\n";
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
        else if (a == "--compiler") {
            if (i + 1 >= args.size()) { cerr << "--compiler expects a value\n"; return 1; }
            compilerPath = args[++i];
        }
        else if (a == "--ldflags") {
            if (i + 1 >= args.size()) { cerr << "--ldflags expects a value\n"; return 1; }
            ldflags = args[++i];
        }
        else if (a == "-o") {
            if (i + 1 >= args.size()) { cerr << "-o expects a value\n"; return 1; }
            outputName = args[++i];
        }
        else if (a == "-O0" || a == "-O1" || a == "-O2" || a == "-O3") {
            optLevel = a;
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
            cerr << "IR file: " << irFilename << "\n";
            cerr << "Compiler: " << compilerPath << "\n";
            cerr << "Opt level: " << optLevel << "\n";
            if (!ldflags.empty()) cerr << "Ldflags: " << ldflags << "\n";
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

        StandardLibrary::initialize(codegen);
        
        ast->codegen(codegen);

        if (printIR) {
            codegen.printIR();
        }

        codegen.printIRToFile(irFilename);

        if (emitIROnly) {
            if (!keepIR) {
                
            }
            cout << "Emitted IR to " << irFilename << "\n";
            return 0;
        }

        string cmd = escapeShellArg(compilerPath) + " -w " + optLevel + " " + escapeShellArg(irFilename) +
                     " -o " + escapeShellArg(outputName);

        if (!ldflags.empty()) {
            cmd += " " + ldflags;
        }

        if (!runCommand(cmd, verbose)) {
            cerr << "Failed to compile executable (compiler returned non-zero).\n";
            if (!keepIR) {
                cerr << "IR retained at: " << irFilename << " for debugging.\n";
            }
            return 1;
        }

        if (runAfter) {
            string runCmd = "./" + escapeShellArg(outputName);
            if (verbose) cerr << "[run] " << runCmd << endl;
            int r = system(runCmd.c_str());
            if (r != 0) {
                if (verbose) cerr << "Executed program returned non-zero: " << r << endl;
            }
        }

        if (!keepIR) {
            string cleanupCommand = "rm -f " + escapeShellArg(irFilename) + " >/dev/null 2>&1";
            if (verbose) cerr << "[cleanup] " << cleanupCommand << endl;
            system(cleanupCommand.c_str());
        } else {
            if (verbose) cerr << "Keeping IR file: " << irFilename << endl;
        }

    } catch (const exception& e) {
        cerr << "Compilation error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
