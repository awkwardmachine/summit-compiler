#include "format_utils.h"
#include <stdexcept>

std::string buildFormatSpecifiers(const std::string& formatStr) {
    std::string formatSpecifiers;
    
    size_t pos = 0;
    size_t lastPos = 0;
    
    while ((pos = formatStr.find('{', lastPos)) != std::string::npos) {
        if (pos > lastPos) {
            formatSpecifiers += formatStr.substr(lastPos, pos - lastPos);
        }
        
        size_t endPos = formatStr.find('}', pos);
        if (endPos == std::string::npos) {
            throw std::runtime_error("Unclosed '{' in format string");
        }
        
        formatSpecifiers += "%s";
        
        lastPos = endPos + 1;
    }

    if (lastPos < formatStr.length()) {
        formatSpecifiers += formatStr.substr(lastPos);
    }
    
    return formatSpecifiers;
}

std::vector<std::string> extractExpressionStrings(const std::string& formatStr) {
    std::vector<std::string> expressions;
    
    size_t pos = 0;
    while ((pos = formatStr.find('{', pos)) != std::string::npos) {
        size_t endPos = formatStr.find('}', pos);
        if (endPos == std::string::npos) {
            throw std::runtime_error("Unclosed '{' in format string");
        }
        
        std::string exprStr = formatStr.substr(pos + 1, endPos - pos - 1);
        expressions.push_back(exprStr);
        
        pos = endPos + 1;
    }
    
    return expressions;
}