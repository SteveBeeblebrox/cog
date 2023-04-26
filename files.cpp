#include "files.h"

#include <fstream> 
#include <string>

#include "filesystem.h"
#include "console.hpp"
#include "commands.h"

namespace fs = FILESYSTEM_NAMESPACE;
using namespace std;
using namespace console;

namespace files {
    /// @brief Creates a directory named NAME if it does not exist already
    void mkdir(const std::string NAME) {
        if (!fs::is_directory(NAME) || !fs::exists(NAME)) {
            fs::create_directory(NAME);
        }
    }

    /// @brief Opens a file, writes to it, and closes the file
    void fwrite(const std::string NAME, const std::string TEXT) {
        ofstream stream(NAME);
        if(stream.fail()) {
            throw runtime_error(format("Error creating file \"%s\"", commands::escape_quotes(NAME).c_str()));
        }
        stream << TEXT;
        stream.close();
    }
    
    /// @brief Opens a file, reads it in its entirety to a string, and closes the file
    std::string fread(const std::string NAME) {
        ifstream stream(NAME);
        if(stream.fail()) {
            throw runtime_error(format("Error reading file \"%s\"", commands::escape_quotes(NAME).c_str()));
        }
        stringstream buffer;
        buffer << stream.rdbuf();
        stream.close();

        return buffer.str();
    }

    /// @brief Returns true if NAME exists and is a file, false otherwise
    bool fexists(const std::string NAME) {
        return !fs::is_directory(NAME) && fs::exists(NAME);
    }

    /// @brief Throws an error if NAME contains null or / (Some platforms may enforce additional rules not checked here)
    void validate_fname(const std::string NAME) {
        // String literals/const char* break with a null in them
        if(NAME.find_first_of(string("/") + '\0') != string::npos) {
            throw runtime_error(format("Invalid file name \"%s\"", commands::escape_quotes(NAME).c_str()));
        }
    }
}