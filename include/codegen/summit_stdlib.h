#ifndef STANDARD_LIBRARY_H
#define STANDARD_LIBRARY_H

#include "CodeGen.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/IRBuilder.h>
#include <string>
#include <vector>

class StandardLibrary {
public:
    static void initialize(CodeGen& context);

private:
    static void createStdModule(CodeGen& context);
    static void createIOModule(CodeGen& context);

    static void createPrintlnFunction(CodeGen& context);

    static void createStringConversionFunctions(CodeGen& context);
    static void createIntToStringFunction(CodeGen& context, 
                                          const std::string& name,
                                          llvm::Type* intType,
                                          const char* format);
    static void createFloatToStringFunction(CodeGen& context, 
                                            const std::string& name,
                                            llvm::Type* floatType,
                                            const char* format);
    static void createBoolToStringFunction(CodeGen& context,
                                           const std::string& name,
                                           llvm::Type* boolType);

    static llvm::Function* createModuleFunction(CodeGen& context,
                                                const std::string& name,
                                                llvm::Type* returnType,
                                                const std::vector<llvm::Type*>& paramTypes);
};

#endif
