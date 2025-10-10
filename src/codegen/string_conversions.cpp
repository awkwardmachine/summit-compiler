#include "string_conversions.h"
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <iostream>
#include <algorithm>

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

    llvm::Value* convertToString(CodeGen& context, llvm::Value* value) {
        auto& builder = context.getBuilder();
        auto& module = context.getModule();
        auto& llvmContext = context.getContext();
        
        std::cout << "DEBUG convertToString: value type = ";
        value->getType()->print(llvm::errs());
        std::cout << std::endl;
        
        if (value->getType()->isPointerTy()) {
            std::cout << "DEBUG: It's a pointer type, returning as-is" << std::endl;
            return value;
        }

        auto mallocFunc = module.getFunction("malloc");
        if (!mallocFunc) {
            auto* i8Ptr = llvm::PointerType::get(llvm::Type::getInt8Ty(llvmContext), 0);
            auto* sizeT = llvm::Type::getInt64Ty(llvmContext);
            auto mallocType = llvm::FunctionType::get(i8Ptr, {sizeT}, false);
            mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", &module);
        }
        
        auto sprintfFunc = module.getFunction("sprintf");
        if (!sprintfFunc) {
            auto* i8Ptr = llvm::PointerType::get(llvm::Type::getInt8Ty(llvmContext), 0);
            auto* i32 = llvm::Type::getInt32Ty(llvmContext);
            std::vector<llvm::Type*> paramTypes = {i8Ptr, i8Ptr};
            auto funcType = llvm::FunctionType::get(i32, paramTypes, true);
            sprintfFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "sprintf", &module);
        }

        const int BUFFER_SIZE = 64;
        auto* buffer = builder.CreateCall(mallocFunc, {llvm::ConstantInt::get(builder.getInt64Ty(), BUFFER_SIZE)});

        llvm::Value* formatStr;
        
        if (value->getType()->isIntegerTy()) {
            unsigned bitWidth = value->getType()->getIntegerBitWidth();
            std::cout << "DEBUG: Integer type, bitWidth = " << bitWidth << std::endl;
            
            if (bitWidth < 64) {
                std::cout << "DEBUG: Extending from " << bitWidth << " to 64 bits" << std::endl;
                value = builder.CreateSExt(value, llvm::Type::getInt64Ty(llvmContext));
            }
            
            formatStr = builder.CreateGlobalStringPtr("%lld");
            std::cout << "DEBUG: Using format %lld for integer" << std::endl;
            std::vector<llvm::Value*> sprintfArgs = {buffer, formatStr, value};
            builder.CreateCall(sprintfFunc, sprintfArgs);
        }
        else {
            formatStr = builder.CreateGlobalStringPtr("%p");
            std::vector<llvm::Value*> sprintfArgs = {buffer, formatStr, value};
            builder.CreateCall(sprintfFunc, sprintfArgs);
        }
        
        return buffer;
    }
}