#ifndef COMMANDS_H
#define COMMANDS_H
#include <string>
#include <vector>

#include "configstring/stringlib.h"

namespace commands {
    struct CommandResult {
        std::string output;
        int status;
    };
    /// @brief Run COMMAND with ARGS and return exit code (stdout is written to console)
    int run(const std::string COMMAND,  const std::vector<std::string> ARGS = std::vector<std::string>());
    /// @brief Run COMMAND with ARGS and return exit code and command stdout (stdout is not written to console)
    CommandResult run_and_read(const std::string COMMAND, const std::vector<std::string> ARGS = std::vector<std::string>());
    /// @brief Check that command COMMAND can be run by checking that COMMAND --version exists with code 0
    void assert_command_exists(const std::string COMMAND, const std::string WHICH_KEY, const std::string ARG = "--version");
    /// @brief Escapes " -> \\" (One literal backslash and one quote)
    std::string escape_quotes(const std::string ARG);
    /// @brief Escapes <space> -> \\<space> (One literal backslash and one space)
    std::string escape_spaces(const std::string ARG);
    /// @brief Gets the value of enviornment variable NAME 
    std::string get_env_var(const std::string NAME);
    /// @brief Sets the value of local enviornment variable NAME to VALUE
    bool set_env_var(const std::string NAME, const std::string VALUE);

    /// @brief Combines strings to form a path env variable separating entries with ':' (';' on Windows) 
    std::string concat_path(const std::string PATH);

    /// @brief Combines strings to form a path env variable separating entries with ':' (';' on Windows) 
    template<typename... Args>
    std::string concat_path(const std::string PATH, Args... args)
    {
        if(configstring::stringlib::str_is_empty(PATH))
            return concat_path(args...);
        else
            return PATH + 
#ifdef WINDOWS
                ";"
#else
                ":"
#endif
             + concat_path(args...);
    }
}
#endif