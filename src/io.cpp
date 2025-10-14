#include <iostream>
#include <string>
#include <cstring>
#include <limits>

extern "C" {

void io_print_str(const char* str) {
    if (str) {
        std::cout << str;
    }
}

void io_println_str(const char* str) {
    if (str) {
        std::cout << str << std::endl;
    } else {
        std::cout << std::endl;
    }
}

char* io_readln() {
    std::string line;
    std::getline(std::cin, line);
    char* buffer = new char[line.size() + 1];
    std::memcpy(buffer, line.c_str(), line.size() + 1);
    return buffer;
}

int io_readint() {
    int value = 0;
    std::cin >> value;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return value;
}

}
