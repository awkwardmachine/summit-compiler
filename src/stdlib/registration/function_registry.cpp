#include "function_registry.h"
#include "stdlib/functions/io/println_function.h"
#include "stdlib/functions/io/print_function.h"

void FunctionRegistry::registerAllFunctions(StdLibManager& manager) {
    manager.registerFunction(std::make_unique<PrintlnFunction>());
    manager.registerFunction(std::make_unique<PrintFunction>());
}