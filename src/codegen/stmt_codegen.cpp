#include "stmt_codegen.h"
#include "codegen.h"
#include "ast/ast.h"
#include <llvm/IR/Verifier.h>
#include "codegen/bounds.h"

using namespace llvm;
using namespace AST;

llvm::Value* StatementCodeGen::codegenVariableDecl(CodeGen& context, VariableDecl& decl) {
    auto varType = context.getLLVMType(decl.getType());
    if (!varType) {
        throw std::runtime_error("Unknown variable type");
    }
    
    auto& builder = context.getBuilder();
    auto currentBlock = builder.GetInsertBlock();
    if (!currentBlock) {
        throw std::runtime_error("No current basic block");
    }
    
    auto currentFunction = currentBlock->getParent();
    if (!currentFunction) {
        throw std::runtime_error("No current function");
    }
    
    auto entryBlock = &currentFunction->getEntryBlock();
    builder.SetInsertPoint(entryBlock);
    
    auto alloca = builder.CreateAlloca(varType, nullptr, decl.getName());
    
    builder.SetInsertPoint(currentBlock);
    
    if (decl.getValue()) {
        auto value = decl.getValue()->codegen(context);
        if (!value) {
            throw std::runtime_error("Failed to generate value for variable: " + decl.getName());
        }
        
        if (decl.getType() == VarType::STRING) {
            if (!value->getType()->isPointerTy()) {
                throw std::runtime_error("String variable must be initialized with a string literal");
            }
            builder.CreateStore(value, alloca);
        } else {
            if (auto numberExpr = dynamic_cast<NumberExpr*>(decl.getValue().get())) {
                const BigInt& bigValue = numberExpr->getValue();
                
                if (!TypeBounds::checkBounds(decl.getType(), bigValue)) {
                    throw std::runtime_error(
                        "Value " + bigValue.toString() + " out of bounds for type " + 
                        TypeBounds::getTypeName(decl.getType()) + ". " +
                        "Valid range: " + TypeBounds::getTypeRange(decl.getType())
                    );
                }
            }
            
            if (value->getType() != varType) {
                if (value->getType()->getIntegerBitWidth() < varType->getIntegerBitWidth()) {
                    value = builder.CreateSExt(value, varType);
                } else {
                    value = builder.CreateTrunc(value, varType);
                }
            }
            
            builder.CreateStore(value, alloca);
        }
    }
    
    auto& namedValues = context.getNamedValues();
    namedValues[decl.getName()] = alloca;
    
    auto& variableTypes = context.getVariableTypes();
    variableTypes[decl.getName()] = decl.getType();
    
    if (decl.getIsConst()) {
        auto& constVariables = context.getConstVariables();
        constVariables.insert(decl.getName());
    }
    
    return alloca;
}
llvm::Value* StatementCodeGen::codegenAssignment(CodeGen& context, AssignmentStmt& stmt) {
    auto& namedValues = context.getNamedValues();
    auto var = namedValues[stmt.getName()];
    if (!var) {
        throw std::runtime_error("Unknown variable: " + stmt.getName());
    }
    
    auto& constVariables = context.getConstVariables();
    if (constVariables.find(stmt.getName()) != constVariables.end()) {
        throw std::runtime_error("Cannot assign to const variable: " + stmt.getName());
    }

    auto value = stmt.getValue()->codegen(context);

    auto& builder = context.getBuilder();
    builder.CreateStore(value, var);
    return value;
}

llvm::Value* StatementCodeGen::codegenExprStmt(CodeGen& context, ExprStmt& stmt) {
    return stmt.getExpr()->codegen(context);
}

llvm::Value* StatementCodeGen::codegenProgram(CodeGen& context, Program& program) {
    auto& llvmContext = context.getContext();
    auto& builder = context.getBuilder();
    
    auto mainType = FunctionType::get(Type::getInt32Ty(llvmContext), false);
    auto mainFunc = Function::Create(mainType, Function::ExternalLinkage, "main", &context.getModule());
    
    auto block = BasicBlock::Create(llvmContext, "entry", mainFunc);
    builder.SetInsertPoint(block);
    
    try {
        for (auto& stmt : program.getStatements()) {
            stmt->codegen(context);
        }
    } catch (const std::runtime_error& e) {
        builder.CreateRet(ConstantInt::get(Type::getInt32Ty(llvmContext), 1));
        verifyFunction(*mainFunc);
        throw e;
    }

    builder.CreateRet(ConstantInt::get(Type::getInt32Ty(llvmContext), 0));
    
    if (verifyFunction(*mainFunc, &llvm::errs())) {
        throw std::runtime_error("Generated function failed verification");
    }
    
    return mainFunc;
}