#include "stmt_codegen.h"
#include "codegen.h"
#include "ast/ast.h"
#include <llvm/IR/Verifier.h>
#include <llvm/IR/BasicBlock.h>
#include "codegen/bounds.h"

using namespace llvm;
using namespace AST;

llvm::Value* StatementCodeGen::codegenVariableDecl(CodeGen& context, VariableDecl& decl) {
    if (decl.getType() == VarType::VOID) {
        throw std::runtime_error("Cannot declare variable of type 'void'");
    }

    auto varType = context.getLLVMType(decl.getType());
    if (!varType) {
        throw std::runtime_error("Unknown variable type");
    }
    
    auto& builder = context.getBuilder();
    
    auto currentFunction = builder.GetInsertBlock()->getParent();
    if (!currentFunction) {
        throw std::runtime_error("No current function");
    }
    
    IRBuilder<> entryBuilder(&currentFunction->getEntryBlock(), 
                           currentFunction->getEntryBlock().begin());
    auto alloca = entryBuilder.CreateAlloca(varType, nullptr, decl.getName());

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
        }
        else if (decl.getType() == VarType::UINT0) {
            auto zero = ConstantInt::get(Type::getInt1Ty(context.getContext()), 0);
            builder.CreateStore(zero, alloca);
        }
        else {
            bool isFloatType = (decl.getType() == VarType::FLOAT32 || decl.getType() == VarType::FLOAT64);

            if (isFloatType) {
                if (value->getType()->isIntegerTy()) {
                    value = builder.CreateSIToFP(value, varType);
                } else if (value->getType()->isFPOrFPVectorTy() && value->getType() != varType) {
                    if (decl.getType() == VarType::FLOAT32 && value->getType()->isDoubleTy()) {
                        value = builder.CreateFPTrunc(value, varType);
                    } else if (decl.getType() == VarType::FLOAT64 && value->getType()->isFloatTy()) {
                        value = builder.CreateFPExt(value, varType);
                    }
                }
                else if (auto floatExpr = dynamic_cast<FloatExpr*>(decl.getValue().get())) {
                    if (decl.getType() == VarType::FLOAT32) {
                        value = ConstantFP::get(Type::getFloatTy(context.getContext()), floatExpr->getValue());
                    } else {
                        value = ConstantFP::get(Type::getDoubleTy(context.getContext()), floatExpr->getValue());
                    }
                }
            }
            else if (!isFloatType && value->getType()->isFPOrFPVectorTy()) {
                value = builder.CreateFPToSI(value, varType);

                if (auto floatExpr = dynamic_cast<FloatExpr*>(decl.getValue().get())) {
                    double floatValue = floatExpr->getValue();
                    if (decl.getType() == VarType::INT32 && (floatValue < INT32_MIN || floatValue > INT32_MAX)) {
                        throw std::runtime_error("Float value " + std::to_string(floatValue) + 
                                               " out of bounds for int32");
                    }
                    if (decl.getType() == VarType::INT64 && (floatValue < INT64_MIN || floatValue > INT64_MAX)) {
                        throw std::runtime_error("Float value " + std::to_string(floatValue) + 
                                               " out of bounds for int64");
                    }
                }
            }
            else if (auto numberExpr = dynamic_cast<NumberExpr*>(decl.getValue().get())) {
                const BigInt& bigValue = numberExpr->getValue();
                if (!TypeBounds::checkBounds(decl.getType(), bigValue)) {
                    throw std::runtime_error(
                        "Value " + bigValue.toString() + " out of bounds for type " + 
                        TypeBounds::getTypeName(decl.getType()) + ". " +
                        "Valid range: " + TypeBounds::getTypeRange(decl.getType())
                    );
                }
            }

            if (!isFloatType && value->getType() != varType && value->getType()->isIntegerTy()) {
                if (value->getType()->getIntegerBitWidth() < varType->getIntegerBitWidth()) {
                    value = builder.CreateSExt(value, varType);
                } else {
                    value = builder.CreateTrunc(value, varType);
                }
            }

            if (value->getType() != varType) {
                if (decl.getType() == VarType::FLOAT32 && value->getType()->isDoubleTy()) {
                    value = builder.CreateFPTrunc(value, varType);
                }
                else if (decl.getType() == VarType::FLOAT64 && value->getType()->isFloatTy()) {
                    value = builder.CreateFPExt(value, varType);
                }
                else {
                    std::string expectedType = varType->getTypeID() == Type::FloatTyID ? "float32" :
                                              varType->getTypeID() == Type::DoubleTyID ? "float64" : "integer";
                    std::string actualType = value->getType()->getTypeID() == Type::FloatTyID ? "float32" :
                                            value->getType()->getTypeID() == Type::DoubleTyID ? "float64" : "integer";
                    throw std::runtime_error("Type mismatch in variable declaration for " + decl.getName() + 
                                           ": expected " + expectedType + ", got " + actualType);
                }
            }

            builder.CreateStore(value, alloca);
        }
    }
    else {
        if (decl.getType() == VarType::FLOAT32) {
            auto zero = ConstantFP::get(Type::getFloatTy(context.getContext()), 0.0);
            builder.CreateStore(zero, alloca);
        }
        else if (decl.getType() == VarType::FLOAT64) {
            auto zero = ConstantFP::get(Type::getDoubleTy(context.getContext()), 0.0);
            builder.CreateStore(zero, alloca);
        }
        else if (decl.getType() == VarType::UINT0) {
            auto zero = ConstantInt::get(Type::getInt1Ty(context.getContext()), 0);
            builder.CreateStore(zero, alloca);
        }
        else if (decl.getType() != VarType::STRING) {
            auto zero = ConstantInt::get(varType, 0);
            builder.CreateStore(zero, alloca);
        }
    }

    context.getNamedValues()[decl.getName()] = alloca;
    context.getVariableTypes()[decl.getName()] = decl.getType();

    if (decl.getIsConst()) {
        context.getConstVariables().insert(decl.getName());
    }

    return alloca;
}
llvm::Value* StatementCodeGen::codegenAssignment(CodeGen& context, AssignmentStmt& stmt) {
    auto var = context.lookupVariable(stmt.getName());
    if (!var) {
        throw std::runtime_error("Unknown variable: " + stmt.getName());
    }

    if (context.isVariableConst(stmt.getName())) {
        throw std::runtime_error("Cannot assign to const variable: " + stmt.getName());
    }

    VarType varType = context.lookupVariableType(stmt.getName());
    if (varType == VarType::VOID) {
        throw std::runtime_error("Unknown variable type for: " + stmt.getName());
    }
    
    if (varType == VarType::UINT0) {
        throw std::runtime_error("Cannot assign to uint0 — value is always 0");
    }

    auto value = stmt.getValue()->codegen(context);
    auto& builder = context.getBuilder();
    
    llvm::Type* expectedType = context.getLLVMType(varType);
    
    if (value->getType() != expectedType) {
        if (expectedType->isFPOrFPVectorTy() && value->getType()->isFPOrFPVectorTy()) {
            if (varType == VarType::FLOAT32 && value->getType()->isDoubleTy()) {
                value = builder.CreateFPTrunc(value, expectedType);
            } else if (varType == VarType::FLOAT64 && value->getType()->isFloatTy()) {
                value = builder.CreateFPExt(value, expectedType);
            }
        }
        else if (expectedType->isFPOrFPVectorTy() && value->getType()->isIntegerTy()) {
            value = builder.CreateSIToFP(value, expectedType);
        }
        else if (expectedType->isIntegerTy() && value->getType()->isFPOrFPVectorTy()) {
            value = builder.CreateFPToSI(value, expectedType);
        }
        else if (expectedType->isIntegerTy() && value->getType()->isIntegerTy()) {
            if (value->getType()->getIntegerBitWidth() < expectedType->getIntegerBitWidth()) {
                value = builder.CreateSExt(value, expectedType);
            } else {
                value = builder.CreateTrunc(value, expectedType);
            }
        }
    }
    
    if (value->getType() != expectedType) {
        throw std::runtime_error("Type mismatch in assignment to variable: " + stmt.getName());
    }

    builder.CreateStore(value, var);
    return value;
}

llvm::Value* StatementCodeGen::codegenExprStmt(CodeGen& context, ExprStmt& stmt) {
    return stmt.getExpr()->codegen(context);
}

llvm::Value* StatementCodeGen::codegenProgram(CodeGen& context, Program& program) {
    auto& builder = context.getBuilder();
    auto& module = context.getModule();
    auto& llvmContext = context.getContext();
    
    auto mainFuncType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(llvmContext), 
        false
    );
    auto mainFunc = llvm::Function::Create(
        mainFuncType,
        llvm::Function::ExternalLinkage,
        "main",
        &module
    );
    
    auto entryBlock = llvm::BasicBlock::Create(llvmContext, "entry", mainFunc);
    builder.SetInsertPoint(entryBlock);
    
    for (auto& stmt : program.getStatements()) {
        stmt->codegen(context);
    }
    
    builder.CreateRet(llvm::ConstantInt::get(llvmContext, llvm::APInt(32, 0)));
    
    if (llvm::verifyFunction(*mainFunc, &llvm::errs())) {
        throw std::runtime_error("Generated function failed verification");
    }
    
    return mainFunc;
}

llvm::Value* StatementCodeGen::codegenBlockStmt(CodeGen& context, BlockStmt& stmt) {
    auto& builder = context.getBuilder();

    for (auto& stmt : stmt.getStatements()) {
        stmt->codegen(context);
    }
    
    return nullptr;
}

llvm::Value* StatementCodeGen::codegenIfStmt(CodeGen& context, IfStmt& stmt) {
    auto& builder = context.getBuilder();
    auto& llvmContext = context.getContext();
    
    auto condValue = stmt.getCondition()->codegen(context);
    
    if (!condValue->getType()->isIntegerTy(1)) {
        if (condValue->getType()->isIntegerTy()) {
            condValue = builder.CreateICmpNE(condValue, 
                ConstantInt::get(condValue->getType(), 0));
        } else {
            throw std::runtime_error("If condition must be a boolean or integer type");
        }
    }
    
    auto currentFunction = builder.GetInsertBlock()->getParent();
    
    auto thenBlock = BasicBlock::Create(llvmContext, "then", currentFunction);
    auto mergeBlock = BasicBlock::Create(llvmContext, "ifcont");
    BasicBlock* elseBlock;
    
    if (stmt.getElseBranch()) {
        elseBlock = BasicBlock::Create(llvmContext, "else");
    } else {
        elseBlock = mergeBlock;
    }
    
    builder.CreateCondBr(condValue, thenBlock, elseBlock);

    builder.SetInsertPoint(thenBlock);
    stmt.getThenBranch()->codegen(context);
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(mergeBlock);
    }
    
    if (stmt.getElseBranch()) {
        currentFunction->insert(currentFunction->end(), elseBlock);
        builder.SetInsertPoint(elseBlock);
        stmt.getElseBranch()->codegen(context);
        if (!builder.GetInsertBlock()->getTerminator()) {
            builder.CreateBr(mergeBlock);
        }
    }
    
    currentFunction->insert(currentFunction->end(), mergeBlock);
    builder.SetInsertPoint(mergeBlock);
    
    return nullptr;
}