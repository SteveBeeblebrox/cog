#include "commands.h"

#include <stdio.h>
#include <string>
#include <stdexcept>
#include <vector>
#include <string>

#include "console.hpp"
#include "configstring/stringlib.h"

using namespace std;
using namespace console;

#ifdef WINDOWS
#define popen _popen
#define pclose _pclose
#endif

namespace commands {
    /// @brief Run COMMAND with ARGS and return exit code (stdout is written to console)
    int run(const std::string COMMAND, const std::vector<std::string> ARGS) {
        string tail = "";
        for(const auto ARG : ARGS) {
            tail += format(" \"%s\"", escape_quotes(ARG).c_str());
        }
        return system(format("%s%s", COMMAND.c_str(), tail.c_str()).c_str());
    }
    /// @brief Run COMMAND with ARGS and return exit code and command stdout (stdout is not written to console)
    CommandResult run_and_read(const std::string COMMAND, const std::vector<std::string> ARGS) {
        string tail = "";
        for(const auto ARG : ARGS) {
            tail += format(" \"%s\"", escape_quotes(ARG).c_str());
        }
        
        char buffer[128];
        string result = "";
        FILE* pipe = popen(format("%s%s", COMMAND.c_str(), tail.c_str()).c_str(), "r");
        if (!pipe) {
            throw runtime_error(format("Failed to run command \"%s\"", COMMAND.c_str()));
        }
        try {
            while (fgets(buffer, sizeof buffer, pipe) != NULL) {
                result += buffer;
            }
        } catch (...) {
            pclose(pipe);
            throw runtime_error(format("Failed to run command \"%s\"", COMMAND.c_str()));
        }
        return CommandResult {result, pclose(pipe)};
    }
    /// @brief Check that command COMMAND can be run by checking that COMMAND --version exists with code 0
    void assert_command_exists(const std::string COMMAND, const std::string WHICH_KEY, const std::string ARG) {
        if(commands::run_and_read(COMMAND, vector<string> {ARG}).status != 0) {
            throw runtime_error(format("Command \"%s\" does not exist. Either set which.%s in project.cfg or install %s", COMMAND.c_str(), WHICH_KEY.c_str(), COMMAND.c_str()));
        }
    }
    /// @brief Excapes " -> \\" (One literal backslash and one quote)
    std::string escape_quotes(const std::string ARG) {
        return configstring::stringlib::str_replace(ARG,"\"","\\\"");
    }
}