#include "codegen.h"
#include "builtins.h"
#include "expr_codegen.h"
#include "stmt_codegen.h"
#include <llvm/IR/Verifier.h>
#include "codegen/bounds.h"
#include "ast.h"
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/LegacyPassManager.h>
#include <system_error>
#include <cstdlib>
#include <fstream>
#include <filesystem>

/* Using LLVM and AST namespaces */
using namespace llvm;
using namespace AST;

/* Constructor that sets up LLVM context, builder, and module */
CodeGen::CodeGen() {
    llvmContext = std::make_unique<LLVMContext>();
    irBuilder = std::make_unique<IRBuilder<>>(*llvmContext);
    llvmModule = std::make_unique<Module>("summit", *llvmContext);

    enterScope();
}

/* Convert AST types to LLVM types */
llvm::Type* CodeGen::getLLVMType(AST::VarType type, const std::string& structName) {
    if (type == AST::VarType::STRUCT) {
        if (structName.empty()) {
            throw std::runtime_error("Struct type requires a struct name");
        }
        auto structType = getStructType(structName);
        if (!structType) {
            throw std::runtime_error("Unknown struct type: " + structName);
        }
        return structType;
    }

    auto& context = getContext();
    switch (type) {
        case AST::VarType::BOOL: return Type::getInt1Ty(context);
        case AST::VarType::INT4: return Type::getInt8Ty(context);
        case AST::VarType::INT8: return Type::getInt8Ty(context);
        case AST::VarType::INT12: return Type::getInt16Ty(context);
        case AST::VarType::INT16: return Type::getInt16Ty(context);
        case AST::VarType::INT24: return Type::getInt32Ty(context);
        case AST::VarType::INT32: return Type::getInt32Ty(context);
        case AST::VarType::INT48: return Type::getInt64Ty(context);
        case AST::VarType::INT64: return Type::getInt64Ty(context);
        case AST::VarType::UINT4: return Type::getInt8Ty(context);
        case AST::VarType::UINT8: return Type::getInt8Ty(context);
        case AST::VarType::UINT12: return Type::getInt16Ty(context);
        case AST::VarType::UINT16: return Type::getInt16Ty(context);
        case AST::VarType::UINT24: return Type::getInt32Ty(context);
        case AST::VarType::UINT32: return Type::getInt32Ty(context);
        case AST::VarType::UINT48: return Type::getInt64Ty(context);
        case AST::VarType::UINT64: return Type::getInt64Ty(context);
        case AST::VarType::UINT0: return Type::getInt1Ty(context);
        case AST::VarType::FLOAT32: return Type::getFloatTy(context);
        case AST::VarType::FLOAT64: return Type::getDoubleTy(context);
        case AST::VarType::STRING: return PointerType::get(Type::getInt8Ty(context), 0);
        case AST::VarType::VOID: return Type::getVoidTy(context);
        case AST::VarType::MODULE: 
            return StructType::create(context, "module_t");
        default: throw std::runtime_error("Unknown type");
    }
}
/* Enter a new scope */
void CodeGen::enterScope() {
    std::unordered_map<std::string, llvm::Value*> newNamedValues;
    std::unordered_map<std::string, AST::VarType> newVariableTypes;
    std::unordered_set<std::string> newConstVariables;
    
    for (const auto& globalName : globalVariables) {
        for (auto it = namedValuesStack.rbegin(); it != namedValuesStack.rend(); ++it) {
            auto found = it->find(globalName);
            if (found != it->end()) {
                newNamedValues[globalName] = found->second;
                break;
            }
        }
        
        for (auto it = variableTypesStack.rbegin(); it != variableTypesStack.rend(); ++it) {
            auto found = it->find(globalName);
            if (found != it->end()) {
                newVariableTypes[globalName] = found->second;
                break;
            }
        }

        for (auto it = constVariablesStack.rbegin(); it != constVariablesStack.rend(); ++it) {
            if (it->find(globalName) != it->end()) {
                newConstVariables.insert(globalName);
                break;
            }
        }
        
        std::cout << "DEBUG enterScope: Carrying over global '" << globalName << "' to new scope" << std::endl;
    }
    
    std::cout << "DEBUG enterScope: Carried " << newNamedValues.size() << " global variables to new scope" << std::endl;
    
    namedValuesStack.push_back(newNamedValues);
    variableTypesStack.push_back(newVariableTypes);
    constVariablesStack.push_back(newConstVariables);
    
    std::cout << "DEBUG enterScope: Created new scope with " << newNamedValues.size() 
              << " total variables" << std::endl;
}

/* Exit current scope */
void CodeGen::exitScope() {
    if (!namedValuesStack.empty()) {
        namedValuesStack.pop_back();
        variableTypesStack.pop_back();
        constVariablesStack.pop_back();
    }
}
/* Get variables in current scope */
std::unordered_map<std::string, llvm::Value*>& CodeGen::getNamedValues() {
    if (namedValuesStack.empty()) throw std::runtime_error("No active scope");
    return namedValuesStack.back();
}

/* Get variable types in current scope */
std::unordered_map<std::string, AST::VarType>& CodeGen::getVariableTypes() {
    if (variableTypesStack.empty()) throw std::runtime_error("No active scope");
    return variableTypesStack.back();
}

/* Get const variables in current scope */
std::unordered_set<std::string>& CodeGen::getConstVariables() {
    if (constVariablesStack.empty()) throw std::runtime_error("No active scope");
    return constVariablesStack.back();
}

/* Look up variable value from inner to outer scope */
llvm::Value* CodeGen::lookupVariable(const std::string& name) {
    for (auto it = namedValuesStack.rbegin(); it != namedValuesStack.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second;
    }
    return nullptr;
}

/* Look up variable type from inner to outer scope */
AST::VarType CodeGen::lookupVariableType(const std::string& name) {
    for (auto it = variableTypesStack.rbegin(); it != variableTypesStack.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second;
    }
    return AST::VarType::VOID;
}

/* Check if a variable is const */
bool CodeGen::isVariableConst(const std::string& name) {
    for (auto it = constVariablesStack.rbegin(); it != constVariablesStack.rend(); ++it) {
        if (it->find(name) != it->end()) return true;
    }
    return false;
}

void CodeGen::registerStructType(const std::string& name, llvm::StructType* type, 
                                const std::vector<std::pair<std::string, AST::VarType>>& fields) {
    structTypes[name] = type;
    structFields_[name] = fields;
    
    auto& fieldMap = structFieldIndices[name];
    for (size_t i = 0; i < fields.size(); i++) {
        fieldMap[fields[i].first] = i;
    }
}

int CodeGen::getStructFieldIndex(const std::string& structName, const std::string& fieldName) {
    auto structIt = structFieldIndices.find(structName);
    if (structIt == structFieldIndices.end()) {
        return -1;
    }
    
    auto fieldIt = structIt->second.find(fieldName);
    if (fieldIt == structIt->second.end()) {
        return -1;
    }
    
    return fieldIt->second;
}

llvm::Type* CodeGen::getStructType(const std::string& name) {
    auto it = structTypes.find(name);
    if (it != structTypes.end()) {
        return it->second;
    }
    
    llvm::StructType* structType = llvm::StructType::create(getContext(), name);
    structTypes[name] = structType;
    return structType;
}

/* Create builtin functions */
void CodeGen::createPrintlnFunction() {
    Builtins::createPrintfFunction(*this);
    Builtins::createPrintlnFunction(*this);
}

/* Pass expression code generation */
llvm::Value* CodeGen::codegen(StringExpr& expr) {
    return ExpressionCodeGen::codegenString(*this, expr);
}
llvm::Value* CodeGen::codegen(FormatStringExpr& expr) {
    return ExpressionCodeGen::codegenFormatString(*this, expr);
}
llvm::Value* CodeGen::codegen(NumberExpr& expr) {
    return ExpressionCodeGen::codegenNumber(*this, expr);
}
llvm::Value* CodeGen::codegen(FloatExpr& expr) {
    return ExpressionCodeGen::codegenFloat(*this, expr);
}
llvm::Value* CodeGen::codegen(BooleanExpr& expr) {
    return ExpressionCodeGen::codegenBoolean(*this, expr);
}
llvm::Value* CodeGen::codegen(VariableExpr& expr) {
    return ExpressionCodeGen::codegenVariable(*this, expr);
}
llvm::Value* CodeGen::codegen(BinaryExpr& expr) {
    return ExpressionCodeGen::codegenBinary(*this, expr);
}
llvm::Value* CodeGen::codegen(CallExpr& expr) {
    return ExpressionCodeGen::codegenCall(*this, expr);
}
llvm::Value* CodeGen::codegen(CastExpr& expr) {
    return ExpressionCodeGen::codegenCast(*this, expr);
}
llvm::Value* CodeGen::codegen(UnaryExpr& expr) {
    return ExpressionCodeGen::codegenUnary(*this, expr);
}

/* Pass statement code generation */
llvm::Value* CodeGen::codegen(VariableDecl& decl) {
    return StatementCodeGen::codegenVariableDecl(*this, decl);
}
llvm::Value* CodeGen::codegen(AssignmentStmt& stmt) {
    return StatementCodeGen::codegenAssignment(*this, stmt);
}
llvm::Value* CodeGen::codegen(BlockStmt& stmt) {
    return StatementCodeGen::codegenBlockStmt(*this, stmt);
}
llvm::Value* CodeGen::codegen(IfStmt& stmt) {
    return StatementCodeGen::codegenIfStmt(*this, stmt);
}
llvm::Value* CodeGen::codegen(ExprStmt& stmt) {
    return StatementCodeGen::codegenExprStmt(*this, stmt);
}
llvm::Value* CodeGen::codegen(Program& program) {
    return StatementCodeGen::codegenProgram(*this, program);
}
llvm::Value* CodeGen::codegen(FunctionStmt& stmt) {
    return StatementCodeGen::codegenFunctionStmt(*this, stmt);
}
llvm::Value* CodeGen::codegen(ReturnStmt& stmt) {
    return StatementCodeGen::codegenReturnStmt(*this, stmt);
}
llvm::Value* CodeGen::codegen(WhileStmt& stmt) {
    return StatementCodeGen::codegenWhileStmt(*this, stmt);
}
llvm::Value* CodeGen::codegen(ForLoopStmt& stmt) {
    return StatementCodeGen::codegenForLoopStmt(*this, stmt);
}

llvm::Value* CodeGen::codegen(AST::ModuleExpr& expr) {
    return ExpressionCodeGen::codegenModule(*this, expr);
}

llvm::Value* CodeGen::codegen(AST::MemberAccessExpr& expr) {
    return ExpressionCodeGen::codegenMemberAccess(*this, expr);
}

llvm::Value* CodeGen::codegen(AST::EnumValueExpr& expr) {
    return ExpressionCodeGen::codegenEnumValue(*this, expr);
}

llvm::Value* CodeGen::codegen(AST::EnumDecl& expr) {
    return StatementCodeGen::codegenEnumDecl(*this, expr);
}

llvm::Value* CodeGen::codegen(AST::BreakStmt& expr) {
    return StatementCodeGen::codegenBreakStmt(*this, expr);
}

llvm::Value* CodeGen::codegen(AST::ContinueStmt& expr) {
    return StatementCodeGen::codegenContinueStmt(*this, expr);
}

llvm::Value* CodeGen::codegen(AST::StructDecl& expr) {
    return StatementCodeGen::codegenStructDecl(*this, expr);
}

llvm::Value* CodeGen::codegen(AST::StructLiteralExpr& expr) {
    return ExpressionCodeGen::codegenStructLiteral(*this, expr);
}

void CodeGen::registerModuleAlias(const std::string& alias, const std::string& actualModuleName, llvm::Value* moduleValue) {
    moduleAliases[alias] = actualModuleName;
    moduleReferences[alias] = moduleValue;
    moduleIdentities[alias] = actualModuleName;
    
    std::cout << "DEBUG: Registered module alias: " << alias << " -> " << actualModuleName << std::endl;
}

std::string CodeGen::resolveModuleAlias(const std::string& name) const {
    auto aliasIt = moduleAliases.find(name);
    if (aliasIt != moduleAliases.end()) {
        return aliasIt->second;
    }
    
    auto identityIt = moduleIdentities.find(name);
    if (identityIt != moduleIdentities.end()) {
        return identityIt->second;
    }
    
    return "";
}

/* Print LLVM IR to stdout */
void CodeGen::printIR() {
    llvmModule->print(outs(), nullptr);
}

void CodeGen::printIRToFile(const std::string& filename) {
    std::error_code EC;
    llvm::raw_fd_ostream out(filename, EC);
    if (EC) throw std::runtime_error("Could not open file: " + filename);
    llvmModule->print(out, nullptr);
}
bool CodeGen::compileToExecutable(const std::string& outputFilename, bool verbose, const std::string& targetTriple, bool noStdlib) {
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    std::string triple = targetTriple;
    if (triple.empty()) {
#if defined(_WIN32)
        triple = "x86_64-w64-windows-gnu";
#elif defined(__APPLE__)
        triple = "x86_64-apple-darwin";
#else
        triple = "x86_64-pc-linux-gnu";
#endif
    }

    llvmModule->setTargetTriple(triple);

    if (verbose) {
        std::cerr << "Target triple: " << triple << std::endl;
        if (noStdlib) std::cerr << "Standard library: disabled" << std::endl;
    }

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(triple, error);
    if (!target) {
        std::cerr << "Error: " << error << std::endl;
        return false;
    }

    auto cpu = "generic";
    auto features = "";
    llvm::TargetOptions opt;
    auto targetMachine = target->createTargetMachine(triple, cpu, features, opt, llvm::Reloc::PIC_);
    llvmModule->setDataLayout(targetMachine->createDataLayout());

    std::string objFilename = outputFilename + ".o";
    std::error_code EC;
    llvm::raw_fd_ostream dest(objFilename, EC, llvm::sys::fs::OF_None);
    if (EC) {
        std::cerr << "Could not open file: " << EC.message() << std::endl;
        return false;
    }

    llvm::legacy::PassManager pass;
    auto fileType = llvm::CodeGenFileType::ObjectFile;
    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        std::cerr << "TargetMachine can't emit a file of this type" << std::endl;
        return false;
    }

    pass.run(*llvmModule);
    dest.flush();

    if (verbose) std::cerr << "Generated object file: " << objFilename << std::endl;

    bool isWindows = (triple.find("windows") != std::string::npos ||
                      triple.find("mingw") != std::string::npos ||
                      triple.find("win32") != std::string::npos);
    bool isLinux = (triple.find("linux") != std::string::npos);
    bool isMac = (triple.find("darwin") != std::string::npos || triple.find("apple") != std::string::npos);

    std::string stdlibPath, dllPath;
    bool useStdlib = !noStdlib;

    if (useStdlib) {
        const char* envLib = std::getenv("SUMMIT_LIB");
        if (envLib) {
            std::filesystem::path libPath(envLib);
            if (std::filesystem::is_directory(libPath)) {
                if (isWindows) {
                    std::vector<std::string> winLibNames = {"libsummit.lib", "libsummit.a"};
                    for (const auto& libName : winLibNames) {
                        std::filesystem::path fullPath = libPath / libName;
                        if (std::filesystem::exists(fullPath)) { stdlibPath = fullPath.string(); break; }
                    }
                    std::vector<std::string> winDllNames = {"libsummit.dll", "libsummit.dll"};
                    for (const auto& dllName : winDllNames) {
                        std::filesystem::path fullDllPath = libPath / dllName;
                        if (std::filesystem::exists(fullDllPath)) { dllPath = fullDllPath.string(); break; }
                    }
                } else {
                    std::vector<std::string> unixLibNames = {"libsummit.a", "libsummit.so", "libsummit.dylib"};
                    for (const auto& libName : unixLibNames) {
                        std::filesystem::path fullPath = libPath / libName;
                        if (std::filesystem::exists(fullPath)) {
                            stdlibPath = fullPath.string();
                            if (libName.find(".so") != std::string::npos || libName.find(".dylib") != std::string::npos)
                                dllPath = stdlibPath;
                            break;
                        }
                    }
                }
            } else if (std::filesystem::exists(libPath)) {
                stdlibPath = envLib;
                std::string ext = libPath.extension().string();
                if (ext == ".dll" || ext == ".so" || ext == ".dylib") dllPath = envLib;
            }
        }
        
        if (stdlibPath.empty()) {
            std::vector<std::string> searchPaths = {
                "./lib",
                "/usr/local/lib",
                "/usr/lib",
                "/lib"
            };
            
            std::vector<std::string> libNames;
            if (isWindows) {
                libNames = {"libsummit.lib", "libsummit.a"};
            } else if (isLinux) {
                libNames = {"libsummit.so", "libsummit.a"};
            } else if (isMac) {
                libNames = {"libsummit.dylib", "libsummit.a"};
            } else {
                libNames = {"libsummit.a", "libsummit.so", "libsummit.dylib"};
            }
            
            for (const auto& searchPath : searchPaths) {
                for (const auto& libName : libNames) {
                    std::filesystem::path fullPath = std::filesystem::path(searchPath) / libName;
                    if (std::filesystem::exists(fullPath)) {
                        stdlibPath = fullPath.string();
                        std::string ext = fullPath.extension().string();
                        if (ext == ".so" || ext == ".dylib") dllPath = stdlibPath;
                        break;
                    }
                }
                if (!stdlibPath.empty()) break;
            }
        }
        
        if (stdlibPath.empty()) {
            std::cerr << "Warning: Standard library not found.\n";
            std::cerr << "Set SUMMIT_LIB to point to the library directory or specific library file.\n";
            std::cerr << "Searched in: ./lib/, /usr/local/lib/, /usr/lib/, /lib/\n";
            return false;
        }
        if (verbose) { 
            std::cerr << "Using standard library: " << stdlibPath << std::endl; 
            if (!dllPath.empty()) std::cerr << "Shared library: " << dllPath << std::endl; 
        }
    }

    std::string linkCmd;
    int result = 0;

    if (isWindows) {
        std::string exeName = outputFilename;
        if (exeName.length() < 4 || exeName.substr(exeName.length()-4) != ".exe") 
            exeName += ".exe";

        std::string mainRenameCmd = "objcopy --redefine-sym main=ProgramMain \"" + objFilename + "\"";
        if (verbose) std::cerr << "Renaming main function: " << mainRenameCmd << std::endl;
        int renameResult = std::system(mainRenameCmd.c_str());
        
        if (renameResult != 0 && verbose) {
            std::cerr << "Warning: Failed to rename main function (objcopy not available or failed)" << std::endl;
        }

        std::string wrapperSource = 
            "#include <windows.h>\n"
            "#include <cstdio>\n"
            "#include <io.h>\n"
            "#include <fcntl.h>\n"
            "#include <iostream>\n"
            "\n"
            "// forward declaration of the actual program main\n"
            "extern \"C\" int ProgramMain();\n"
            "\n"
            "int main() {\n"
            "    // Allocate a console for this application\n"
            "    if (AllocConsole()) {\n"
            "        // redirect stdout\n"
            "        HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);\n"
            "        int fdStdout = _open_osfhandle((intptr_t)hStdout, _O_TEXT);\n"
            "        FILE* fpStdout = _fdopen(fdStdout, \"w\");\n"
            "        *stdout = *fpStdout;\n"
            "        setvbuf(stdout, NULL, _IONBF, 0);\n"
            "\n"
            "        // redirect stderr\n"
            "        HANDLE hStderr = GetStdHandle(STD_ERROR_HANDLE);\n"
            "        int fdStderr = _open_osfhandle((intptr_t)hStderr, _O_TEXT);\n"
            "        FILE* fpStderr = _fdopen(fdStderr, \"w\");\n"
            "        *stderr = *fpStderr;\n"
            "        setvbuf(stderr, NULL, _IONBF, 0);\n"
            "\n"
            "        // redirect stdin\n"
            "        HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);\n"
            "        int fdStdin = _open_osfhandle((intptr_t)hStdin, _O_TEXT);\n"
            "        FILE* fpStdin = _fdopen(fdStdin, \"r\");\n"
            "        *stdin = *fpStdin;\n"
            "        setvbuf(stdin, NULL, _IONBF, 0);\n"
            "    }\n"
            "\n"
            "    // call the actual program\n"
            "    int result = ProgramMain();\n"
            "\n"
            "    // keep console open for a moment to see output\n"
            "    std::cout << \"\\nPress Enter to exit...\";\n"
            "    std::cin.get();\n"
            "\n"
            "    return result;\n"
            "}\n";

        std::string wrapperFile = "console_wrapper.cpp";
        std::ofstream outFile(wrapperFile);
        if (!outFile) {
            std::cerr << "Error: Could not create console wrapper file" << std::endl;
            std::remove(objFilename.c_str());
            return false;
        }
        outFile << wrapperSource;
        outFile.close();

        std::string wrapperObj = "console_wrapper.o";
        std::string compileWrapperCmd = "g++ -c \"" + wrapperFile + "\" -o \"" + wrapperObj + "\"";
        if (verbose) std::cerr << "Compiling wrapper: " << compileWrapperCmd << std::endl;
        int compileResult = std::system(compileWrapperCmd.c_str());
        
        if (compileResult != 0) {
            std::cerr << "Error: Failed to compile console wrapper" << std::endl;
            std::remove(wrapperFile.c_str());
            std::remove(objFilename.c_str());
            return false;
        }

        if (useStdlib) {
            linkCmd = "g++ -mconsole -o \"" + exeName + "\" \"" + objFilename + "\" \"" + wrapperObj + "\" \"" + stdlibPath + "\"";
        } else {
            linkCmd = "g++ -mconsole -o \"" + exeName + "\" \"" + objFilename + "\" \"" + wrapperObj + "\"";
        }

        if (!dllPath.empty()) {
            std::filesystem::path dllDir = std::filesystem::path(dllPath).parent_path();
            linkCmd += " -L\"" + dllDir.string() + "\"";
        }

        linkCmd += " -luser32 -lkernel32 -lgdi32 -ladvapi32";

        if (verbose) {
            std::cerr << "Linking command: " << linkCmd << std::endl;
        }

        result = std::system(linkCmd.c_str());

        std::remove(wrapperFile.c_str());
        std::remove(wrapperObj.c_str());

        if (result == 0 && !dllPath.empty()) {
            std::filesystem::path exeDir = std::filesystem::path(exeName).parent_path();
            std::filesystem::path targetDll = exeDir / std::filesystem::path(dllPath).filename();
            try { 
                if (!std::filesystem::exists(targetDll)) 
                    std::filesystem::copy_file(dllPath, targetDll, std::filesystem::copy_options::overwrite_existing); 
            } catch (...) { 
                if (verbose) std::cerr << "Warning: Failed to copy DLL\n"; 
            }
        }

        if (result == 0) {
            std::cout << "Successfully created Windows console executable: " << exeName << std::endl;
            std::cout << "This executable will open a console window when run." << std::endl;
        }

    } else { 
        if (useStdlib) {
            std::filesystem::path libPath(stdlibPath);
            std::string libDir = libPath.parent_path().string();
            std::string libName = libPath.filename().string();
            
            std::string baseLibName = libName;
            if (baseLibName.find("lib") == 0) {
                baseLibName = baseLibName.substr(3);
            }
            size_t dotPos = baseLibName.find_last_of('.');
            if (dotPos != std::string::npos) {
                baseLibName = baseLibName.substr(0, dotPos);
            }
            
            linkCmd = "clang++ -o \"" + outputFilename + "\" \"" + objFilename + "\"";
            linkCmd += " -L\"" + libDir + "\"";
            linkCmd += " -l" + baseLibName;
            
            if (!dllPath.empty()) {
                linkCmd += " -Wl,-rpath,\"" + libDir + "\"";
            }
            
            if (isLinux) {
                linkCmd += " -lm -ldl -lpthread";
            } else if (isMac) {
                linkCmd += " -framework Foundation";
            }
        } else {
            linkCmd = "clang++ -o \"" + outputFilename + "\" \"" + objFilename + "\"";
            if (isLinux) {
                linkCmd += " -lm -ldl -lpthread";
            }
        }
        
        if (verbose) linkCmd += " -v";
        if (verbose) std::cerr << "Linking command: " << linkCmd << std::endl;
        result = std::system(linkCmd.c_str());

        if (result == 0) {
            std::cout << "Successfully created executable: " << outputFilename << std::endl;
        }
    }

    std::remove(objFilename.c_str());

    if (result != 0) {
        std::cerr << "Linking failed" << std::endl;
        return false;
    }

    return true;
}

const std::vector<std::pair<std::string, AST::VarType>>& CodeGen::getStructFields(const std::string& structName) const {
    static std::vector<std::pair<std::string, AST::VarType>> empty;
    
    auto it = structFields_.find(structName);
    if (it != structFields_.end()) {
        return it->second;
    }
    return empty;
}
