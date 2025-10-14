#include "stdlib_manager.h"
#include "stdlib/registration/module_registry.h"
#include "stdlib/registration/function_registry.h"

bool StdLibManager::initialized = false;
bool StdLibManager::stdlibEnabled = true;

StdLibManager& StdLibManager::getInstance() {
    static StdLibManager instance;
    return instance;
}

void StdLibManager::registerModule(ModulePtr module) {
    if (module && stdlibEnabled) {
        modules.push_back(std::move(module));
    }
}

void StdLibManager::registerFunction(FunctionPtr function) {
    if (function && stdlibEnabled) {
        functions.push_back(std::move(function));
    }
}

ModuleInterface* StdLibManager::findModuleHandler(const std::string& moduleName) {
    if (!stdlibEnabled) {
        return nullptr;
    }
    
    auto it = moduleCache.find(moduleName);
    if (it != moduleCache.end()) {
        return it->second;
    }
    
    for (auto& module : modules) {
        if (module->handlesModule(moduleName)) {
            moduleCache[moduleName] = module.get();
            return module.get();
        }
    }
    return nullptr;
}

FunctionInterface* StdLibManager::findFunctionHandler(const std::string& functionName, size_t argCount) {
    if (!stdlibEnabled) {
        return nullptr;
    }
    
    std::string key = functionName + "_" + std::to_string(argCount);
    auto it = functionCache.find(key);
    if (it != functionCache.end()) {
        return it->second;
    }
    
    for (auto& function : functions) {
        if (function->handlesCall(functionName, argCount)) {
            functionCache[key] = function.get();
            return function.get();
        }
    }
    return nullptr;
}

void StdLibManager::initializeStandardLibrary(bool enableStdlib) {
    if (initialized) {
        return;
    }
    
    stdlibEnabled = enableStdlib;
    initialized = true;
    
    if (!enableStdlib) {
        return;
    }
    
    ModuleRegistry::registerAllModules(*this);
    FunctionRegistry::registerAllFunctions(*this);
}

bool StdLibManager::isStdlibEnabled() {
    return stdlibEnabled;
}

bool StdLibManager::isInitialized() {
    return initialized;
}