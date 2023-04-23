#include "console.hpp"

#include <string>

void console::assert(bool condition, std::string message) {
    if(condition) return;
    eprintlnf("Assertion error: %s", message);
    exit(1);
}