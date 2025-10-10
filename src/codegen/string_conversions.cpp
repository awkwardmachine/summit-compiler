#include "string_conversions.h"
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <iostream>
#include <algorithm>
#include "type_inference.h"
#include "bounds.h"

using namespace llvm;

namespace AST {
    llvm::Value* convertToBinaryString(CodeGen& context, llvm::Value* value) {
        auto& builder = context.getBuilder();
        auto& module = context.getModule();
        auto& llvmContext = context.getContext();
        
        auto mallocFunc = module.getFunction("malloc");
        if (!mallocFunc) {
            auto* i8Ptr = llvm::PointerType::get(llvm::Type::getInt8Ty(llvmContext), 0);
            auto* sizeT = llvm::Type::getInt64Ty(llvmContext);
            auto mallocType = llvm::FunctionType::get(i8Ptr, {sizeT}, false);
            mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", &module);
        }
        
        const int BUFFER_SIZE = 128;
        auto* buffer = builder.CreateCall(mallocFunc, {llvm::ConstantInt::get(builder.getInt64Ty(), BUFFER_SIZE)});
        
        if (value->getType()->isIntegerTy()) {
            llvm::Value* intValue = value;
            unsigned bitWidth = value->getType()->getIntegerBitWidth();
            if (bitWidth < 64) {
                intValue = builder.CreateZExt(value, llvm::Type::getInt64Ty(llvmContext));
            }
            
            auto strcpyFunc = module.getFunction("strcpy");
            if (!strcpyFunc) {
                auto* i8Ptr = llvm::PointerType::get(llvm::Type::getInt8Ty(llvmContext), 0);
                std::vector<llvm::Type*> paramTypes = {i8Ptr, i8Ptr};
                auto funcType = llvm::FunctionType::get(i8Ptr, paramTypes, false);
                strcpyFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "strcpy", &module);
            }
            
            auto prefix = builder.CreateGlobalStringPtr("0b");
            builder.CreateCall(strcpyFunc, {buffer, prefix});
            
            auto zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvmContext), 0);
            auto oneChar = llvm::ConstantInt::get(llvm::Type::getInt8Ty(llvmContext), '1');
            auto zeroChar = llvm::ConstantInt::get(llvm::Type::getInt8Ty(llvmContext), '0');
            
            for (int i = 63; i >= 0; i--) {
                auto bitMask = llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvmContext), 1ULL << i);
                auto bit = builder.CreateAnd(intValue, bitMask);
                auto isBitSet = builder.CreateICmpNE(bit, zero);
                
                auto digitChar = builder.CreateSelect(isBitSet, oneChar, zeroChar);

                auto pos = llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvmContext), (63 - i) + 2);
                auto charPtr = builder.CreateGEP(llvm::Type::getInt8Ty(llvmContext), buffer, pos);
                builder.CreateStore(digitChar, charPtr);
            }
            
            auto nullTermPtr = builder.CreateGEP(llvm::Type::getInt8Ty(llvmContext), buffer, 
                                                llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvmContext), 66));
            builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt8Ty(llvmContext), 0), nullTermPtr);
            
            return buffer;
        }
        
        return buffer;
    }

    llvm::Value* convertToDecimalString(CodeGen& context, llvm::Value* value) {
        auto& builder = context.getBuilder();
        auto& module = context.getModule();
        auto& llvmContext = context.getContext();
        
        auto mallocFunc = module.getFunction("malloc");
        if (!mallocFunc) {
            auto* i8Ptr = llvm::PointerType::get(llvm::Type::getInt8Ty(llvmContext), 0);
            auto* sizeT = llvm::Type::getInt64Ty(llvmContext);
            auto mallocType = llvm::FunctionType::get(i8Ptr, {sizeT}, false);
            mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", &module);
        }
        
        const int BUFFER_SIZE = 64;
        auto* buffer = builder.CreateCall(mallocFunc, {llvm::ConstantInt::get(builder.getInt64Ty(), BUFFER_SIZE)});
        
        if (value->getType()->isIntegerTy()) {
            llvm::Value* intValue = value;
            unsigned bitWidth = value->getType()->getIntegerBitWidth();
            
            if (bitWidth < 64) {
                intValue = builder.CreateZExt(value, llvm::Type::getInt64Ty(llvmContext));
            }
            
            auto sprintfFunc = module.getFunction("sprintf");
            if (!sprintfFunc) {
                auto* i8Ptr = llvm::PointerType::get(llvm::Type::getInt8Ty(llvmContext), 0);
                auto* i32 = llvm::Type::getInt32Ty(llvmContext);
                std::vector<llvm::Type*> paramTypes = {i8Ptr, i8Ptr};
                auto funcType = llvm::FunctionType::get(i32, paramTypes, true);
                sprintfFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "sprintf", &module);
            }
            
            auto highBitMask = llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvmContext), 1ULL << 63);
            auto highBitSet = builder.CreateICmpNE(builder.CreateAnd(intValue, highBitMask), 
                                                  llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvmContext), 0));
            
            auto formatStrUnsigned = builder.CreateGlobalStringPtr("%llu");
            auto formatStrSigned = builder.CreateGlobalStringPtr("%lld");
            auto formatStr = builder.CreateSelect(highBitSet, formatStrUnsigned, formatStrSigned);
            
            builder.CreateCall(sprintfFunc, {buffer, formatStr, intValue});
            
            return buffer;
        } else if (value->getType()->isFloatTy() || value->getType()->isDoubleTy()) {
            auto sprintfFunc = module.getFunction("sprintf");
            if (!sprintfFunc) {
                auto* i8Ptr = llvm::PointerType::get(llvm::Type::getInt8Ty(llvmContext), 0);
                auto* i32 = llvm::Type::getInt32Ty(llvmContext);
                std::vector<llvm::Type*> paramTypes = {i8Ptr, i8Ptr};
                auto funcType = llvm::FunctionType::get(i32, paramTypes, true);
                sprintfFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "sprintf", &module);
            }
            
            if (value->getType()->isFloatTy()) {
                auto formatStr = builder.CreateGlobalStringPtr("%.6f");
                builder.CreateCall(sprintfFunc, {buffer, formatStr, value});
            } else {
                auto formatStr = builder.CreateGlobalStringPtr("%.15lf");
                builder.CreateCall(sprintfFunc, {buffer, formatStr, value});
            }
            
            return buffer;
        }
        
        return buffer;
    }

    llvm::Value* AST::convertToString(CodeGen& context, llvm::Value* value) {
        auto& builder = context.getBuilder();
        auto& module = context.getModule();
        auto& llvmContext = context.getContext();

        if (value->getType()->isPointerTy()) {
            return value;
        }

        auto sprintfFunc = module.getFunction("sprintf");
        if (!sprintfFunc) {
            auto* i32 = Type::getInt32Ty(llvmContext);
            auto* i8Ptr = PointerType::get(Type::getInt8Ty(llvmContext), 0);
            auto sprintfType = FunctionType::get(i32, {i8Ptr, i8Ptr}, true);
            sprintfFunc = Function::Create(sprintfType, Function::ExternalLinkage, "sprintf", &module);
        }
        
        auto* i8 = Type::getInt8Ty(llvmContext);
        auto* bufferSize = ConstantInt::get(Type::getInt64Ty(llvmContext), 64);
        auto* buffer = builder.CreateAlloca(i8, bufferSize, "str_buffer");
        
        std::string formatStr;
        llvm::Value* valueToConvert = value;
        
        if (value->getType()->isIntegerTy()) {
            unsigned bitWidth = value->getType()->getIntegerBitWidth();

            VarType sourceType = AST::inferSourceType(value, context);
            bool isUnsigned = TypeBounds::isUnsignedType(sourceType);

            if (bitWidth == 1) {
                auto trueStr = builder.CreateGlobalStringPtr("true");
                auto falseStr = builder.CreateGlobalStringPtr("false");
                return builder.CreateSelect(value, trueStr, falseStr);
            }
            else if (bitWidth < 32) {
                if (isUnsigned) {
                    valueToConvert = builder.CreateZExt(value, Type::getInt32Ty(llvmContext));
                } else {
                    valueToConvert = builder.CreateSExt(value, Type::getInt32Ty(llvmContext));
                }
                formatStr = isUnsigned ? "%u" : "%d";
            }
            else if (bitWidth == 32) {
                formatStr = isUnsigned ? "%u" : "%d";
            }
            else if (bitWidth == 64) {
                formatStr = isUnsigned ? "%llu" : "%lld";
            }
            else {
                if (bitWidth > 64) {
                    valueToConvert = builder.CreateTrunc(value, Type::getInt64Ty(llvmContext));
                } else {
                    if (isUnsigned) {
                        valueToConvert = builder.CreateZExt(value, Type::getInt64Ty(llvmContext));
                    } else {
                        valueToConvert = builder.CreateSExt(value, Type::getInt64Ty(llvmContext));
                    }
                }
                formatStr = isUnsigned ? "%llu" : "%lld";
            }
        }
        else if (value->getType()->isFloatTy()) {
            valueToConvert = builder.CreateFPExt(value, Type::getDoubleTy(llvmContext));
            formatStr = "%g";
        }
        else if (value->getType()->isDoubleTy()) {
            formatStr = "%g";
        }
        else {
            throw std::runtime_error("Cannot convert this type to string");
        }
        
        auto formatStrPtr = builder.CreateGlobalStringPtr(formatStr);
        
        builder.CreateCall(sprintfFunc, {buffer, formatStrPtr, valueToConvert});
        
        return buffer;
    }
}