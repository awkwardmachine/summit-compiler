#include "codegen.h"
#include "builtins.h"
#include "expr_codegen.h"
#include "stmt_codegen.h"
#include <llvm/IR/Verifier.h>
#include "codegen/bounds.h"
#include "ast/ast.h"
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/LegacyPassManager.h>
#include <system_error>
#include <cstdlib>

/* Using LLVM and AST namespaces */
using namespace llvm;
using namespace AST;

/* Constructor that sets up LLVM context, builder, and module */
CodeGen::CodeGen() {
    llvmContext = std::make_unique<LLVMContext>();
    irBuilder = std::make_unique<IRBuilder<>>(*llvmContext);
    llvmModule = std::make_unique<Module>("summit", *llvmContext);

    // Start global scope
    enterScope();
}

/* Convert AST types to LLVM types */
llvm::Type* CodeGen::getLLVMType(AST::VarType type) {
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
    namedValuesStack.push_back({});
    variableTypesStack.push_back({});
    constVariablesStack.push_back({});
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

/* Print LLVM IR to stdout */
void CodeGen::printIR() {
    llvmModule->print(outs(), nullptr);
}

/* Print LLVM IR to file */
void CodeGen::printIRToFile(const std::string& filename) {
    std::error_code EC;
    llvm::raw_fd_ostream out(filename, EC);
    if (EC) throw std::runtime_error("Could not open file: " + filename);
    llvmModule->print(out, nullptr);
}

bool CodeGen::compileToExecutable(const std::string& outputFilename, bool verbose, const std::string& targetTriple) {
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
    auto targetMachine = target->createTargetMachine(
        triple, cpu, features, opt, llvm::Reloc::PIC_);

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

    if (verbose) {
        std::cerr << "Generated object file: " << objFilename << std::endl;
    }

    std::string linkCmd;
    bool isWindows = (triple.find("windows") != std::string::npos || 
                     triple.find("mingw") != std::string::npos ||
                     triple.find("win32") != std::string::npos);
    
    if (isWindows) {
        std::string exeName = outputFilename;
        if (exeName.length() < 4 || exeName.substr(exeName.length() - 4) != ".exe") {
            exeName += ".exe";
        }
        linkCmd = "clang++ -target " + triple + " -o " + exeName + " " + objFilename;
        if (verbose) linkCmd += " -v";
    } else {
        linkCmd = "clang++ -target " + triple + " -o " + outputFilename + " " + objFilename;
        if (verbose) linkCmd += " -v";
    }

    if (verbose) {
        std::cerr << "Linking: " << linkCmd << std::endl;
    }

    int result = std::system(linkCmd.c_str());
    
    std::remove(objFilename.c_str());

    return result == 0;
}