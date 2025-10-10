#pragma once
#include <string>
#include <vector>
#include <stdexcept>

std::string buildFormatSpecifiers(const std::string& formatStr);
std::vector<std::string> extractExpressionStrings(const std::string& formatStr);