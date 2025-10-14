#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include "module_interface.h"
#include "function_interface.h"

class StdLibManager {
private:
    std::vector<ModulePtr> modules;
    std::vector<FunctionPtr> functions;
    std::unordered_map<std::string, ModuleInterface*> moduleCache;
    std::unordered_map<std::string, FunctionInterface*> functionCache;
    static bool initialized;
    static bool stdlibEnabled;
    
public:
    static StdLibManager& getInstance();
    
    void registerModule(ModulePtr module);
    void registerFunction(FunctionPtr function);
    
    ModuleInterface* findModuleHandler(const std::string& moduleName);
    FunctionInterface* findFunctionHandler(const std::string& functionName, size_t argCount);
    
    // Modified to accept enable flag
    void initializeStandardLibrary(bool enableStdlib = true);
    
    // Check if stdlib is enabled
    static bool isStdlibEnabled();
    
    // Check if already initialized
    static bool isInitialized();
};