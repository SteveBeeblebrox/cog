#ifndef CONSOLE_HPP
#define CONSOLE_HPP

#include <iostream>
#include <string>
#include <sstream>

#define _STR(x) #x
#define STR(x) _STR(x)
#define ASSERT(condition) console::assert(condition, "Assertion \"" #condition "\" in file \"" __FILE__ "\" on line " STR(__LINE__) " failed!")

namespace console {
    void assert(bool condition, std::string message = "Assertion failed!");
    
    /// @brief Format a string using standard modifiers like %s and %d. Note that %s needs a const char* not a std::string
    template<typename... Args>
    std::string format(std::string format, Args... args) {
        const char* cstrFmt = format.c_str();
        int length = std::snprintf(nullptr, 0, cstrFmt, args...);
        assert(length >= 0);

        char* buffer = new char[length + 1];
        std::snprintf(buffer, length + 1, cstrFmt, args...);

        std::string formatted(buffer);
        delete[] buffer;
        return formatted;
    }
    namespace {
        template<class T>
        std::string from(const T& t)
        {
            std::stringstream stream;
            stream << t;
            return stream.str();
        }
    }
    template<typename M, typename... Args>
    void printf(M message, Args... args) {
        std::cout << format(from(message), args...);
    }
    template<typename M, typename... Args>
    void printlnf(M message, Args... args) {
        std::cout << format(from(message), args...) << std::endl;
    }

    template<typename M, typename... Args>
    void eprintf(M message, Args... args) {
        std::cerr << format(from(message), args...);
    }
    template<typename M, typename... Args>
    void eprintlnf(M message, Args... args) {
        std::cerr << format(from(message), args...) << std::endl;
    }
}

#endif
