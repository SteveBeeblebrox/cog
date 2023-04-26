#ifndef FILES_H
#define FILES_H
#include <string>

namespace files {
    /// @brief Creates a directory named NAME if it does not exist already
    void mkdir(const std::string NAME);

    /// @brief Opens a file, writes to it, and closes the file
    void fwrite(const std::string NAME, const std::string TEXT);
    
    /// @brief Opens a file, reads it in its entirety to a string, and closes the file
    std::string fread(const std::string NAME);

    /// @brief Returns true if NAME exists and is a file, false otherwise
    bool fexists(const std::string NAME);

   /// @brief Throws an error if NAME contains NUL or / (Some platforms may enforce additional rules not checked here)
    void validate_fname(const std::string NAME);
}
#endif