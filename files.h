#ifndef FILES_H
#define FILES_H
#include <string>

namespace files {
    /// @brief Creates a directory named name if it does not exist already
    void mkdir(std::string name);

    /// @brief Opens a file, writes to it, and closes the file
    void fwrite(std::string name, std::string text);
    
    /// @brief Opens a file, reads it in its entirety to a string, and closes the file
    std::string fread(std::string name);
}
#endif