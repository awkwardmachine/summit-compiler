#ifndef ERROR_UTILS_H
#define ERROR_UTILS_H

#include <string>
#include <stdexcept>

enum class ErrorSeverity {
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class SyntaxError : public std::runtime_error {
public:
    SyntaxError(const std::string& message, size_t line, size_t column, const std::string& sourceLine);
    const char* what() const noexcept override;

private:
    std::string formattedMessage;
};

namespace ErrorUtils {
    std::string colorize(const std::string& text, const std::string& colorCode);
    std::string formatError(const std::string& message, size_t line, size_t column, const std::string& sourceLine);
}

#endif
