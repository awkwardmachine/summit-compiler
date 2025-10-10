#pragma once
#include "codegen.h"

namespace Builtins {
    void createPrintfFunction(CodeGen& context);
    void createPrintlnFunction(CodeGen& context);
}