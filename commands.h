#ifndef COMMANDS_H
#define COMMANDS_H
#include <string>
#include <vector>

namespace commands {
    struct CommandResult {
        std::string output;
        int status;
    };
    int run(const char* command,  const std::vector<std::string> ARGS = std::vector<std::string>());
    CommandResult run_and_read(const char* command, const std::vector<std::string> ARGS = std::vector<std::string>());
}
#endif