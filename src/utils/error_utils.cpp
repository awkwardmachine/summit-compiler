#include "utils/error_utils.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace std;

static const string RED = "\033[38;2;255;85;85m";
static const string YELLOW = "\033[38;2;255;220;100m";
static const string GREEN = "\033[38;2;140;255;140m";
static const string CYAN = "\033[38;2;120;200;255m";
static const string WHITE = "\033[38;2;240;240;240m";
static const string GRAY = "\033[38;2;150;150;150m";
static const string BOLD = "\033[1m";
static const string RESET = "\033[0m";
static const string DIM = "\033[2m";

SyntaxError::SyntaxError(const string& message, size_t line, size_t column, const string& sourceLine)
    : runtime_error(message),
      formattedMessage(ErrorUtils::formatError(message, line, column, sourceLine)) {}

const char* SyntaxError::what() const noexcept {
    return formattedMessage.c_str();
}

namespace ErrorUtils {

string colorize(const string& text, const string& colorCode) {
    return colorCode + text + RESET;
}

string inferFixSuggestion(const string& message) {
    string lowerMsg = message;
    transform(lowerMsg.begin(), lowerMsg.end(), lowerMsg.begin(), ::tolower);

    if (lowerMsg.find("expected ';'") != string::npos)
        return "Did you forget to end your statement with " + colorize(";", YELLOW) + "?";

    if (lowerMsg.find("expected ':'") != string::npos)
        return "You might have meant to declare a type like " + colorize("var x: int8 = 5;", CYAN) + ".";

    if (lowerMsg.find("expected '='") != string::npos)
        return "Initializer missing? Try " + colorize("= value;", CYAN) + " after your type.";

    if (lowerMsg.find("expected variable name") != string::npos)
        return "Add an identifier after " + colorize("var", CYAN) + " or " + colorize("const", CYAN) + ".";

    if (lowerMsg.find("expected expression") != string::npos)
        return "You may have written an incomplete statement or missing parentheses.";

    if (lowerMsg.find("unexpected token") != string::npos)
        return "Remove or replace the invalid token.";

    if (lowerMsg.find("unmatched") != string::npos)
        return "Check if all " + colorize("(", YELLOW) + " and " + colorize(")", YELLOW) + " are balanced.";

    if (lowerMsg.find("type") != string::npos)
        return "Make sure the variable type is valid: " + colorize("int8, int16, int32, int64, uint0, string", CYAN) + ".";

    if (lowerMsg.find("expected ')'") != string::npos)
        return "Missing closing parenthesis for function call or expression.";

    if (lowerMsg.find("expected '('") != string::npos)
        return "Missing opening parenthesis for function call or expression.";

    return "Recheck syntax near this area, something doesn't line up right.";
}

string formatError(const string& message, size_t line, size_t column, const string& sourceLine) {
    stringstream ss;

    ss << "\n"
       << BOLD << RED
       << "╔════════════════════════════════════════════════════════════════════╗\n"
       << "║" << RESET << " " << BOLD << RED << "✖ Syntax Error" << RESET << "  "
       << WHITE << message << RESET << "\n"
       << BOLD << RED
       << "╚════════════════════════════════════════════════════════════════════╝"
       << RESET << "\n";

    ss << BOLD << GRAY
       << "────────────────────────────────────────────────────────────────────"
       << RESET << "\n";

    ss << CYAN << " --> " << RESET
       << "line " << BOLD << line << RESET
       << CYAN << ", column " << BOLD << column << RESET << "\n";

    ss << GRAY << setw(4) << right << line << " | " << RESET << sourceLine << "\n";
    ss << GRAY << "     | " << RESET
       << string(column - 1, ' ')
       << YELLOW << BOLD << " ^" << RESET
       << DIM << " expected here" << RESET << "\n";

    string suggestion = inferFixSuggestion(message);
    ss << "\n" << GREEN << "Possible fix: " << RESET << suggestion << "\n";

    ss << BOLD << GRAY
       << "────────────────────────────────────────────────────────────────────"
       << RESET << "\n";

    return ss.str();
}

}