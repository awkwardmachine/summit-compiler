#include "module_registry.h"
#include "stdlib/modules/std/std_module.h"
#include "stdlib/modules/io/io_module.h"

void ModuleRegistry::registerAllModules(StdLibManager& manager) {
    manager.registerModule(std::make_unique<StdModule>());
    manager.registerModule(std::make_unique<IOModule>());
}