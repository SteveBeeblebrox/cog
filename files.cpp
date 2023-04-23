#include "files.h"

#include <filesystem>
#include <fstream> 
#include <string>

#include "console.hpp"

namespace fs = std::filesystem;
using namespace std;
using namespace console;

namespace files {
    /// @brief Creates a directory named name if it does not exist already
    void mkdir(std::string name) {
        if (!fs::is_directory(name) || !fs::exists(name)) {
            fs::create_directory(name);
        }
    }

    /// @brief Opens a file, writes to it, and closes the file
    void fwrite(std::string name, std::string text) {
        ofstream stream(name);
        if(stream.fail()) {
            throw runtime_error(format("Error creating file \"%s\"", name));
        }
        stream << text;
        stream.close();
    }
    
    /// @brief Opens a file, reads it in its entirety to a string, and closes the file
    std::string fread(std::string name) {
        ifstream stream(name);
        if(stream.fail()) {
            throw runtime_error(format("Error reading file \"%s\"", name));
        }
        stringstream buffer;
        buffer << stream.rdbuf();
        stream.close();

        return buffer.str();
    }
}