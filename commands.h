#ifndef COMMANDS_H
#define COMMANDS_H
#include <string>
#include <vector>

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
    /// @brief Excapes " -> \\" (One literal backslash and one quote)
    std::string escape_quotes(const std::string ARG);
}
#endif